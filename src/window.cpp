#include "window.h"
#include "ui_window.h"
#include <QFileDialog>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QList>
#include <filesystem>
#include <fstream>

window::window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::window)
    , boarddisplay(nullptr)
    , orig_playback_tooltip("Drag and drop into this window\nOr go to File > Open")
{
    ui->setupUi(this);

    // Startup state changes
    reset_results_display();

    // Qt signal mappings
    connect(ui->play_button, &QPushButton::released, this, &window::play_toggle);
    connect(ui->cam_name_edit, &QLineEdit::editingFinished, this, &window::on_cam_name_change);
    connect(ui->next_frame_button, &QPushButton::released, this, &window::next_frame);
    connect(ui->prev_frame_button, &QPushButton::released, this, &window::prev_frame);
    connect(ui->next_bd_button, &QPushButton::released, this, &window::to_next_board);
    connect(ui->prev_bd_button, &QPushButton::released, this, &window::to_prev_board);
    connect(ui->to_beg_button, &QPushButton::released, this, &window::to_beginning);
    connect(ui->to_end_button, &QPushButton::released, this, &window::to_end);
    connect(ui->update_solution_button, &QPushButton::released, this, &window::update_solution);
    connect(ui->export_prof_button, &QPushButton::released, this, &window::export_profile);
    connect(ui->display_board_button, &QPushButton::released, this, &window::show_board_display);
    connect(ui->sensor_width_edit, &QDoubleSpinBox::valueChanged, this, &window::update_focal_length);
    connect(ui->board_width_edit, &QSpinBox::valueChanged, this, &window::show_board_display);
    connect(ui->board_height_edit, &QSpinBox::valueChanged, this, &window::show_board_display);

    // Edit behavior fixes
    connect(ui->board_width_edit, &QSpinBox::editingFinished, this, &window::clear_edit_focus);
    connect(ui->board_height_edit, &QSpinBox::editingFinished, this, &window::clear_edit_focus);
    connect(ui->cam_name_edit, &QLineEdit::editingFinished, this, &window::clear_edit_focus);
    connect(ui->sensor_width_edit, &QDoubleSpinBox::editingFinished, this, &window::clear_edit_focus);

    // Action mappings
    connect(ui->actionOpen, &QAction::triggered, this, &window::open_file);
    connect(ui->actionNext_frame, &QAction::triggered, this, &window::next_frame);
    connect(ui->actionPrevious_frame, &QAction::triggered, this, &window::prev_frame);
    connect(ui->actionDetect_board_on_current_frame, &QAction::triggered, this, &window::detect_board);
    connect(ui->actionNext_board, &QAction::triggered, this, &window::to_next_board);
    connect(ui->actionPrevious_board, &QAction::triggered, this, &window::to_prev_board);
    connect(ui->actionJump_to_beginning, &QAction::triggered, this, &window::to_beginning);
    connect(ui->actionJump_to_end, &QAction::triggered, this, &window::to_end);

    status_info("Ready.");
}

window::~window()
{
    close_board_display();
    delete ui;
}

void window::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void window::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    std::vector<std::string> filenames;
    std::transform(urls.begin(), urls.end(), std::back_inserter(filenames), [](const QUrl& url) { return url.toLocalFile().toStdString(); });
    if (filenames.empty())
        return;
    attempt_video_load(filenames.at(0));
}

void window::resizeEvent(QResizeEvent* event)
{
    display_current_frame();
}

void window::closeEvent(QCloseEvent* event)
{
    close_board_display();
}

void window::attempt_video_load(std::string path)
{
    cv::VideoCapture new_cap;
    new_cap.open(path);
    if (!new_cap.isOpened()) {
        status_error("File \"" + path + "\" failed to load");
        return;
    }
    status_info("File \"" + path + "\" loaded");
    cap.release();
    cap = new_cap;
    last_file = path;
    init_edit_state();
}

void window::clear_edit_focus()
{
    ui->board_width_edit->clearFocus();
    ui->board_height_edit->clearFocus();
    ui->cam_name_edit->clearFocus();
    ui->sensor_width_edit->clearFocus();
}

void window::play_toggle()
{
    if (playing) {
        ui->play_button->setText("Play");
        status_info("Paused.");
        ui->play_button->setText("Stop");
        status_info("Playing...");
        playing = true;
    }
}

void window::on_playback_select()
{
    status_info("Playback area selected");
}

void window::on_cam_name_change()
{
    QString text = ui->cam_name_edit->text();
    if (text.isEmpty()) {
        ui->cam_name_edit->setText(cam_name.c_str());
        return;
    }
    cam_name = text.toStdString();
}

void window::status_info(std::string msg)
{
    auto& status = ui->statusbar;
    status->setStyleSheet("color: black");
    status->showMessage(msg.c_str());
}

void window::status_warn(std::string msg)
{
    auto& status = ui->statusbar;
    status->setStyleSheet("color: orange");
    status->showMessage(msg.c_str());
}

void window::status_error(std::string msg)
{
    auto& status = ui->statusbar;
    status->setStyleSheet("color: red");
    status->showMessage(msg.c_str());
}

void window::show_board_display()
{
    if (!boarddisplay)
        boarddisplay = new BoardDisplay();
    boarddisplay->set_board_img(ui->board_width_edit->value(), ui->board_height_edit->value());
    boarddisplay->show();
}

void window::close_board_display()
{
    if (boarddisplay)
        delete boarddisplay;
    boarddisplay = nullptr;
}

void window::reset_results_display()
{
    std::string reset_str;
    for (int i = 0; i < result_max_chars; ++i) {
        reset_str += '-';
    }
    ui->avg_err_num->setText(reset_str.c_str());
    ui->sol_cov_num->setText(reset_str.c_str());
    ui->hfov_num->setText(reset_str.c_str());
    ui->norm_fx_num->setText(reset_str.c_str());
    ui->dist_k1_num->setText(reset_str.c_str());
    ui->dist_k2_num->setText(reset_str.c_str());
    ui->dist_k3_num->setText(reset_str.c_str());
    update_total_coverage();
    update_focal_length();
}

void window::display_results()
{
    if (!result.success) {
        reset_results_display();
        return;
    }
    auto corners = get_stored_corners();
    double solution_coverage = 0.0;
    if (result.success)
        solution_coverage = get_combined_area(result.c_corners) / result.src_img_size.area() * 100;
    ui->avg_err_num->setText(QString::number(result.reproj_error, 'f'));
    ui->sol_cov_num->setText(QString::number(solution_coverage, 'f'));
    ui->hfov_num->setText(QString::number(result.h_fov(), 'f'));
    ui->norm_fx_num->setText(QString::number(result.h_ratio(), 'f'));
    ui->dist_k1_num->setText(QString::number(result.cam_Kk.k(0), 'f'));
    ui->dist_k2_num->setText(QString::number(result.cam_Kk.k(1), 'f'));
    ui->dist_k3_num->setText(QString::number(result.cam_Kk.k(2), 'f'));
    update_focal_length();
}

const std::vector<ChessboardCorners> window::get_stored_corners() const
{
    std::vector<ChessboardCorners> corners;
    std::transform(frame_corners.begin(), frame_corners.end(), std::back_inserter(corners), [](auto& c) { return std::move(c.second); });
    return corners;
}

void window::init_edit_state()
{
    frame_corners.clear();
    ui->cam_name_edit->setText(default_cam_name.c_str());
    cam_name = default_cam_name;
    result = CalibrationResult();
    reset_results_display();
    frame_corners.clear();
    if (cap.isOpened()) {
        read_success = cap.read(current_frame);
    }
    display_current_frame();
}

void window::display_current_frame()
{
    if (!read_success) {
        playback_tooltip_mode();
        return;
    }
    playback_display_mode();
    auto disp_size = ui->playback_widget->size();
    int w = disp_size.width();
    int h = disp_size.height();
    double orig_aspect = static_cast<double>(current_frame.cols) / current_frame.rows;
    double target_aspect = static_cast<double>(w) / h;
    cv::Size resize_dims;
    if (orig_aspect > target_aspect) {
        resize_dims.width = w;
        resize_dims.height = static_cast<int>(w / orig_aspect);
    }
    else {
        resize_dims.height = h;
        resize_dims.width = static_cast<int>(h * orig_aspect);
    }
    cv::Mat resize_img;
    cv::Mat display_frame;
    current_frame.copyTo(display_frame);
    int current_pos = this->current_pos();
    if (frame_corners.find(current_pos) != frame_corners.end())
        frame_corners[current_pos].draw(display_frame);
    result.undistort(display_frame);
    cv::resize(display_frame, resize_img, resize_dims);
    cv::Mat letterbox_img(cv::Size(w, h), current_frame.type(), cv::Scalar(0, 0, 0));
    const int t = (h - resize_dims.height) / 2;
    int l = (w - resize_dims.width) / 2;
    resize_img.copyTo(letterbox_img(cv::Rect(l, t, resize_dims.width, resize_dims.height)));
    draw_playback_bar(letterbox_img);
    QImage img = QImage((uchar*)letterbox_img.data, letterbox_img.cols, letterbox_img.rows, letterbox_img.step, QImage::Format_BGR888);
    QPixmap pixmap = QPixmap::fromImage(img);
    ui->playback_display->setPixmap(pixmap);
}

void window::playback_display_mode()
{
    ui->playback_display->setSizePolicy(QSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Ignored
    ));
    ui->playback_display->setScaledContents(true);
}

void window::playback_tooltip_mode()
{
    ui->playback_display->clear();
    ui->playback_display->setText(orig_playback_tooltip.c_str());
    ui->playback_display->setSizePolicy(QSizePolicy(
        QSizePolicy::Maximum,
        QSizePolicy::Maximum
    ));
    ui->playback_display->setScaledContents(false);
}

int window::current_pos()
{
    if (!cap.isOpened())
        return 0;
    return static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES));
}

int window::total_frames()
{
    if (!cap.isOpened())
        return 0;
    return static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
}

bool window::set_pos(int pos)
{
    if (!cap.isOpened())
        return false;
    int current_pos = this->current_pos();
    int total_frames = this->total_frames();
    if (pos < 1 || pos > total_frames)
        return false;
    if (pos - current_pos != 1 && !cap.set(cv::CAP_PROP_POS_FRAMES, pos - 1))
        return false;
    cv::Mat next_frame;
    bool next_success = cap.read(next_frame);
    for (int i = 0; i < 100; ++i) {
        if (!next_success)
            next_success = cap.read(next_frame);
        else
            break;
    }
    if (!next_success) {
        status_error("Read failure on frame " + std::to_string(static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES)) + 1));
        cap.release();
        cv::VideoCapture new_cap;
        new_cap.open(last_file);
        cap = new_cap;
        cap.set(cv::CAP_PROP_POS_FRAMES, current_pos);
        return false;
    }
    read_success = next_success;
    next_frame.copyTo(current_frame);
    return true;
}

void window::next_frame()
{
    if (!set_pos(this->current_pos() + 1))
        return;
    display_current_frame();
    std::stringstream ss;
    ss << "Advanced to frame " << current_pos() << " of " << total_frames();
    status_info(ss.str());
}

void window::prev_frame()
{
    if (!set_pos(this->current_pos() - 1))
        return;
    display_current_frame();
    std::stringstream ss;
    ss << "Reversed to frame " << current_pos() << " of " << total_frames();
    status_info(ss.str());
}

void window::detect_board() {
    if (!(cap.isOpened() && read_success))
        return;
    auto corners = get_corners(current_frame, ui->board_width_edit->value(), ui->board_height_edit->value());
    if (!corners.valid) {
        status_warn("FAILED TO DETECT BOARD: Check width and height settings or try a different frame");
        return;
    }
    frame_corners[current_pos()] = corners;
    update_total_coverage();
    display_current_frame();
}

void window::draw_playback_bar(cv::Mat& img,
    int bar_height,
    int pos_height,
    int pos_width,
    int board_pos_height,
    int board_pos_width,
    double bar_transparency,
    cv::Scalar bar_color,
    cv::Scalar pos_color,
    cv::Scalar board_color) {
    if (!cap.isOpened())
        return;
    int iw = img.cols;
    int ih = img.rows;
    cv::Mat overlay;
    img.copyTo(overlay);
    cv::rectangle(overlay, cv::Point(0, ih - 1 - bar_height), cv::Point(iw - 1, ih - 1), bar_color, -1);
    int current_pos = this->current_pos();
    int total_frames = this->total_frames();
    double norm_pos = static_cast<double>(current_pos) / total_frames;
    int draw_pos = iw * norm_pos;
    cv::line(overlay, cv::Point(draw_pos, ih - 1 - pos_height), cv::Point(draw_pos, ih - 1), pos_color, pos_width);
    for (auto& fc : frame_corners) {
        norm_pos = static_cast<double>(fc.first) / total_frames;
        draw_pos = iw * norm_pos;
        cv::line(overlay, cv::Point(draw_pos, ih - 1 - board_pos_height), cv::Point(draw_pos, ih - 1), board_color, board_pos_width);
    }
    cv::addWeighted(overlay, bar_transparency, img, 1.0 - bar_transparency, 0, img);
}

void window::update_total_coverage()
{
    auto stored_corners = get_stored_corners();
    if (stored_corners.empty() || !cap.isOpened()) {
        std::string reset_str;
        for (int i = 0; i < result_max_chars; ++i) {
            reset_str += '-';
        }
        ui->tot_cov_num->setText(reset_str.c_str());
        return;
    }
    double area = get_combined_area(stored_corners);
    int w = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int h = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int wh = w * h;
    ui->tot_cov_num->setText(QString::number(area / wh * 100, 'f'));
}

void window::update_focal_length()
{
    if (!result.success || !cap.isOpened()) {
        std::string reset_str;
        for (int i = 0; i < result_max_chars; ++i) {
            reset_str += '-';
        }
        ui->focal_length_num->setText(reset_str.c_str());
        return;
    }
    ui->focal_length_num->setText(QString::number(result.focal_length(ui->sensor_width_edit->value()), 'f'));
}

void window::to_next_board()
{
    if (frame_corners.empty())
        return;
    auto next_board = frame_corners.upper_bound(current_pos());
    if (next_board == frame_corners.end() || !set_pos(next_board->first))
        return;
    display_current_frame();
    std::stringstream ss;
    ss << "Jumped to frame " << current_pos();
    status_info(ss.str());
}

void window::to_prev_board()
{
    if (frame_corners.empty())
        return;
    auto next_board = frame_corners.lower_bound(current_pos());
    if (next_board == frame_corners.begin())
        return;
    next_board = std::prev(next_board);
    if (!set_pos(next_board->first))
        return;
    display_current_frame();
    std::stringstream ss;
    ss << "Jumped to frame " << current_pos();
    status_info(ss.str());
}

void window::to_beginning()
{
    if (!set_pos(1))
        return;
    display_current_frame();
    status_info("Jumped to beginning");
}

void window::to_end()
{
    if (!set_pos(total_frames()))
        return;
    display_current_frame();
    status_info("Jumped to end");
}

void window::update_solution()
{
    result = calibrate_camera(get_stored_corners());
    display_results();
    display_current_frame();
}

void window::open_file()
{
    QString fn = QFileDialog::getOpenFileName(this, "Open File", QString::fromStdString(std::filesystem::current_path().string()));
    if (fn.isEmpty() || fn.isNull())
        return;
    attempt_video_load(fn.toStdString());
}

void window::export_profile()
{
    if (!result.success)
        return;
    std::string base_name = ui->cam_name_edit->text().toStdString() + ".txt";
    auto path = std::filesystem::current_path() / base_name;
    QString fn = QFileDialog::getSaveFileName(this, "Export Profile", QString::fromStdString(path.string()), "Text file (*.txt)");
    if (fn.isEmpty() || fn.isNull())
        return;
    std::ofstream out_stream;
    out_stream.open(fn.toStdString(), std::ios::out);
    if (!out_stream.is_open()) {
        status_error("Failed writing to \"" + fn.toStdString() + "\"");
        return;
    }
    out_stream << "cam_name=" << ui->cam_name_edit->text().toStdString() << std::endl;
    out_stream << "sensor_width=" << ui->sensor_width_edit->value() << std::endl;
    out_stream << "focal_length=" << result.focal_length(ui->sensor_width_edit->value()) << std::endl;
    out_stream << "sol_cov=" << get_combined_area(result.c_corners) / result.src_img_size.area() * 100 << std::endl;
    out_stream << "avg_err=" << result.reproj_error << std::endl;
    out_stream << "hfov=" << result.h_fov() << std::endl;
    out_stream << "norm_fx=" << result.h_ratio() << std::endl;
    out_stream << "dist_k1=" << result.cam_Kk.k(0) << std::endl;
    out_stream << "dist_k2=" << result.cam_Kk.k(1) << std::endl;
    out_stream << "dist_k3=" << result.cam_Kk.k(3) << std::endl;
    out_stream.close();
    status_info("Camera profile exported to \"" + fn.toStdString() + "\"");
}
