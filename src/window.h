#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include "calibration.hpp"
#include "boarddisplay.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
    class window;
}
QT_END_NAMESPACE

class window : public QMainWindow
{
    Q_OBJECT

public:
    explicit window(QWidget* parent = nullptr);
    ~window();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;


private:
    Ui::window* ui;
    BoardDisplay* boarddisplay;
    bool playing = false;
    int result_max_chars = 8;
    CalibrationResult result;
    std::map<int, ChessboardCorners> frame_corners;
    const std::string default_cam_name = "Camera";
    std::string cam_name = default_cam_name;
    cv::VideoCapture cap;
    cv::Mat current_frame;
    bool read_success = false;
    const std::string orig_playback_tooltip;
    std::string last_file;

    void status_info(std::string msg);
    void status_warn(std::string msg);
    void status_error(std::string msg);

    void clear_edit_focus();
    void auto_detect_boards();
    void show_board_display();
    void update_board_display();
    void close_board_display();
    void attempt_video_load(std::string path);
    void play_toggle();
    void on_playback_select();
    void on_cam_name_change();
    void reset_results_display();
    void display_results();
    const std::vector<ChessboardCorners> get_stored_corners() const;
    void init_edit_state();
    void display_current_frame();
    void playback_display_mode();
    void playback_tooltip_mode();
    int current_pos();
    int total_frames();
    bool set_pos(int pos);
    void next_frame();
    void prev_frame();
    void detect_board();
    void draw_playback_bar(cv::Mat& img,
        int bar_height = 8,
        int pos_height = 10,
        int pos_width = 2,
        int board_pos_height = 8,
        int board_pos_width = 2,
        double bar_transparency = 0.8,
        cv::Scalar bar_color = cv::Scalar(255, 156, 127),
        cv::Scalar pos_color = cv::Scalar(127, 255, 255),
        cv::Scalar board_color = cv::Scalar(0, 127, 255)
    );
    void update_total_coverage();
    void update_focal_length();
    void to_next_board();
    void to_prev_board();
    void to_beginning();
    void to_end();
    void update_solution();
    void open_file();
    void export_profile();
};

#endif // WINDOW_H
