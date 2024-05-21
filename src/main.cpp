#include <QApplication>
#include "calibration.hpp"
#include "window.h"
#include <opencv2/opencv.hpp>
#include <cmath>

std::vector<cv::Mat> get_frames(cv::VideoCapture& cap, int num_frames) {
    std::vector<cv::Mat> output;
    for (int i = 0; i < num_frames; ++i) {
        bool s = true;
        cv::Mat frame;
        s = cap.read(frame);
        for (int j = 0; j < 100; ++j) {
            if (!s)
                s = cap.read(frame);
            else
                break;
        }
        if (!s)
            break;
        output.push_back(frame);
    }
    return output;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    window w;
    w.show();
    return app.exec();
}