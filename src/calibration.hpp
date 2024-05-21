#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>

struct Kk {
    cv::Matx33d K = cv::Matx33d::eye();
    cv::Matx13d k = cv::Matx13d::zeros();
    const std::vector<float> dist_vector() const;
};

struct ChessboardCorners {
    ChessboardCorners(int width = 0, int height = 0);
    std::vector<cv::Point2f> img_corners;
    std::vector<cv::Point3f> obj_corners;
    cv::Size board_size;
    cv::Size src_img_size;
    // Major 4 corners of chessboard
    const std::vector<cv::Point2f> major4() const;
    bool valid = false;
    void draw(cv::Mat& image) const;
};

struct CalibrationResult {
    Kk cam_Kk;
    double reproj_error = std::numeric_limits<double>::infinity();
    std::vector<ChessboardCorners> c_corners;
    cv::Size src_img_size;
    void undistort(cv::Mat& image) const;
    const double h_ratio() const;
    const double h_fov() const;
    const double focal_length(const double sensor_width = 36) const;
    bool success = false;
};

const ChessboardCorners get_corners(const cv::Mat& image, const int board_width, const int board_height);

const std::vector<ChessboardCorners> get_corners(const std::vector<cv::Mat>& images, const int board_width, const int board_height);

const double get_combined_area(const std::vector<ChessboardCorners>& corners);

const std::vector<ChessboardCorners> find_optimal_corners(const std::vector<ChessboardCorners>& orig_corners, const int num_selections);

const CalibrationResult calibrate_camera(const ChessboardCorners& corners);

const CalibrationResult calibrate_camera(const std::vector<ChessboardCorners>& corners, const int num_selections = 10);

