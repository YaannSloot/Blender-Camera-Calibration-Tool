#include "boarddisplay.hpp"
#include "calibration.hpp"

BoardDisplay::BoardDisplay(QWidget* parent)
	:QWidget(parent)
{
	resize(320, 240);
	setWindowTitle("Board display");
	layout = new QGridLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	board_display = new QLabel();
	layout->addWidget(board_display, 1, 1, Qt::AlignCenter);
	layout->setRowStretch(1, 1);
	layout->setColumnStretch(1, 1);
	layout->setRowMinimumHeight(1, 240);
	layout->setColumnMinimumWidth(1, 320);
	board_display->setScaledContents(true);
	board_display->setAlignment(Qt::AlignCenter);
	board_display->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

void BoardDisplay::set_board_img(const int board_width, const int board_height)
{
	board_img = generate_board_image(board_width, board_height);
	update_display();
}

void BoardDisplay::update_display()
{
	QImage img = QImage((uchar*)board_img.data, board_img.cols, board_img.rows, board_img.step, QImage::Format_Grayscale8);
	QPixmap pixmap = QPixmap::fromImage(img);
	board_display->setPixmap(pixmap.scaled(this->size(), Qt::KeepAspectRatio));
}

void BoardDisplay::resizeEvent(QResizeEvent* event)
{
	update_display();
}
