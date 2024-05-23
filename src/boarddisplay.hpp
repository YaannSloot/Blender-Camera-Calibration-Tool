#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <opencv2/opencv.hpp>

class BoardDisplay : public QWidget {
	Q_OBJECT

public:
	BoardDisplay(QWidget* parent = nullptr);
	void set_board_img(const int board_width, const int board_height);

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	QGridLayout *layout = nullptr;
	QLabel *board_display = nullptr;

	cv::Mat board_img;
	void update_display();
};