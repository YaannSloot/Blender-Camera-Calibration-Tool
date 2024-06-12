#include "calibration.hpp"
#include <algorithm>
#include <execution>
#include <numeric>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace bg = boost::geometry;

using poly_point = bg::model::d2::point_xy<double>;
using polygon = bg::model::polygon<poly_point>;
using poly_set = bg::model::multi_polygon<polygon>;

const std::vector<float> Kk::dist_vector() const {
	std::vector<float> out(5, 0);
	out.at(0) = static_cast<float>(k(0));
	out.at(1) = static_cast<float>(k(1));
	out.at(4) = static_cast<float>(k(2));
	return out;
}

ChessboardCorners::ChessboardCorners(int width, int height)
{
	width = std::abs(width);
	height = std::abs(height);
	board_size = cv::Size(width, height);
	const size_t elements = static_cast<size_t>(width * height);
	this->img_corners.resize(elements, cv::Point2f());
	this->obj_corners.resize(elements, cv::Point3f());
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			const size_t pos = static_cast<size_t>(i * width + j);
			auto& obj_point = this->obj_corners.at(pos);
			obj_point.x = static_cast<float>(j);
			obj_point.y = static_cast<float>(i);
			obj_point.z = 0;
		}
	}
}

const std::vector<cv::Point2f> ChessboardCorners::outer_corners() const
{
	auto& size = this->board_size;
	if(img_corners.size() < size.area())
		return std::vector<cv::Point2f>();
	std::vector<cv::Point2f> result;
	int width = this->board_size.width;
	int height = this->board_size.height;
	for (int i = 0; i < width; ++i) {
		result.push_back(this->img_corners.at(static_cast<size_t>(i)));
	}
	for (int i = 0; i < height; ++i) {
		result.push_back(this->img_corners.at(static_cast<size_t>(i * width + width - 1)));
	}
	for (int i = width - 1; i >= 0; --i) {
		result.push_back(this->img_corners.at(static_cast<size_t>((height - 1) * width + i)));
	}
	for (int i = height - 1; i >= 0; --i) {
		result.push_back(this->img_corners.at(static_cast<size_t>(i * width)));
	}
	return result;
}

void ChessboardCorners::draw(cv::Mat& image) const
{
	if (!this->valid)
		return;
	cv::drawChessboardCorners(image, this->board_size, this->img_corners, true);
	auto corners = this->outer_corners();
	std::vector<cv::Point2i> corners_i;
	std::transform(corners.begin(), corners.end(), std::back_inserter(corners_i),
		[](auto& p) { return cv::Point2i(static_cast<int>(p.x), static_cast<int>(p.y)); });
	cv::polylines(image, { corners_i }, true, cv::Scalar(255, 255, 0), 2);
}

ChessboardCorners ChessboardCorners::get_undistorted(const Kk& cam_Kk)
{
	ChessboardCorners out(*this);
	std::vector<cv::Point2f> undis_points;
	cv::undistortImagePoints(out.img_corners, undis_points, cam_Kk.K, cam_Kk.dist_vector());
	out.img_corners = undis_points;
	return out;
}

void CalibrationResult::undistort(cv::Mat& image) const
{
	if (!this->success)
		return;
	cv::Mat map_a, map_b;
	cv::initUndistortRectifyMap(this->cam_Kk.K, this->cam_Kk.dist_vector(), cv::Mat(), this->cam_Kk.K, cv::Size(image.cols, image.rows), CV_32FC1, map_a, map_b);
	cv::remap(image, image, map_a, map_b, cv::INTER_LINEAR);
}

const double CalibrationResult::h_ratio() const
{
	if (!this->success)
		return 0.0;
	return this->cam_Kk.K(0, 0) / this->src_img_size.width;
}

const double CalibrationResult::h_fov() const
{
	if (!this->success)
		return 0.0;
	return 2 * (100 / boost::math::constants::pi<double>()) * atan(pow(2 * this->h_ratio(), -1));
}

const double CalibrationResult::focal_length(double sensor_width) const
{
	if (!this->success)
		return 0.0;
	return this->h_ratio() * sensor_width;
}

const ChessboardCorners get_corners(const cv::Mat& image, const int board_width, const int board_height) {
	ChessboardCorners result(board_width, board_height);
	cv::Mat gray_img;
	cv::cvtColor(image, gray_img, cv::COLOR_BGR2GRAY);
	const bool success = cv::findChessboardCorners(gray_img, cv::Size(board_width, board_height), result.img_corners,
		cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
	if (!success)
		return result;
	result.valid = true;
	result.src_img_size = cv::Size(image.cols, image.rows);
	const cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
	cv::cornerSubPix(gray_img, result.img_corners, cv::Size(11, 11), cv::Size(-1, -1), criteria);
	return result;
}

const std::vector<ChessboardCorners> get_corners(const std::vector<cv::Mat>& images, const int board_width, const int board_height) {
	std::vector<ChessboardCorners> result(images.size(), ChessboardCorners(0, 0));
	std::vector<size_t> img_idx(images.size());
	std::iota(img_idx.begin(), img_idx.end(), 0);
	std::for_each(std::execution::par_unseq, img_idx.begin(), img_idx.end(), [&](size_t i) {
		result.at(i) = get_corners(images.at(i), board_width, board_height);
	});
	return result;
}

const double get_combined_area(const std::vector<ChessboardCorners>& corners)
{
	poly_set boards;
	poly_set temp;
	for (auto& cdat : corners) {
		auto c = cdat.outer_corners();
		if (c.size() < 4)
			continue;
		polygon poly;
		for (auto& p : c) {
			bg::append(poly.outer(), poly_point(static_cast<double>(p.x), static_cast<double>(p.y)));
		}
		bg::append(poly.outer(), poly_point(static_cast<double>(c.at(0).x), static_cast<double>(c.at(0).y)));
		bg::correct(poly);
		if (boards.empty())
			boards.push_back(poly);
		else {
			bg::union_(boards, poly, temp);
			boards = temp;
			bg::clear(temp);
		}
	}
	if (boards.empty())
		return 0.0;
	return bg::area(boards);
}

const std::vector<ChessboardCorners> find_optimal_corners(const std::vector<ChessboardCorners>& orig_corners, const int num_selections)
{
	std::vector<ChessboardCorners> good_corners;
	std::copy_if(orig_corners.begin(), orig_corners.end(), std::back_inserter(good_corners), [](auto& c) {return c.valid; });
	if (good_corners.empty())
		return std::vector<ChessboardCorners>();
	if (good_corners.size() <= num_selections)
		return good_corners;
	std::vector<size_t> good_idx(good_corners.size());
	std::iota(good_idx.begin(), good_idx.end(), 0);
	std::set<size_t> chosen_idx;
	std::vector<ChessboardCorners> chosen_corners;
	std::mutex lock;
	for (int i = 0; i < num_selections; ++i) {
		std::vector<std::pair<size_t, double>> scores;
		std::for_each(std::execution::par_unseq, good_idx.begin(), good_idx.end(), [&](size_t i) {
			if (chosen_idx.find(i) != chosen_idx.end())
				return;
			auto& c = good_corners.at(i);
			auto corners_copy = chosen_corners;
			corners_copy.push_back(c);
			auto score = get_combined_area(corners_copy);
			std::lock_guard<std::mutex> guard(lock);
			scores.push_back(std::pair<size_t, double>(i, score));
		});
		std::sort(scores.begin(), scores.end(), [](auto& a, auto& b) {return a.second > b.second; });
		chosen_idx.insert(scores.front().first);
		chosen_corners.push_back(good_corners.at(scores.front().first));
	}
	return chosen_corners;
}

const CalibrationResult calibrate_camera(const ChessboardCorners& corners) {
	std::vector<ChessboardCorners> corners_corners;
	corners_corners.push_back(corners);
	return calibrate_camera(corners_corners, -1);
}

const CalibrationResult calibrate_camera(const std::vector<ChessboardCorners>& corners, const int num_selections) {
	CalibrationResult result;
	if (corners.empty())
		return result;
	cv::Size img_size;
	std::vector<ChessboardCorners> good_corners;
	for (auto& c : corners) {
		if (!c.valid)
			continue;
		if (good_corners.empty())
			img_size = c.src_img_size;
		else if (img_size != c.src_img_size)
			continue;
		good_corners.push_back(c);
	}
	if (good_corners.empty())
		return result;
	if (num_selections > 0)
		good_corners = find_optimal_corners(good_corners, num_selections);
	result.c_corners = good_corners;
	result.src_img_size = img_size;
	std::vector<std::vector<cv::Point2f>> imgp;
	std::vector<std::vector<cv::Point3f>> objp;
	for (auto& c : good_corners) {
		imgp.push_back(c.img_corners);
		objp.push_back(c.obj_corners);
	}
	std::vector<float> dist_coeffs;
	std::vector<cv::Mat> rvecs, tvecs;
	cv::calibrateCamera(objp, imgp, img_size, result.cam_Kk.K, dist_coeffs, rvecs, tvecs);
	result.cam_Kk.k(0) = dist_coeffs.at(0);
	result.cam_Kk.k(1) = dist_coeffs.at(1);
	result.cam_Kk.k(2) = dist_coeffs.at(4);
	std::vector<double> singular_error(imgp.size());
	std::vector<size_t> singular_idx(imgp.size());
	std::iota(singular_idx.begin(), singular_idx.end(), 0);
	std::for_each(std::execution::par_unseq, singular_idx.begin(), singular_idx.end(), [&](size_t i) {
		std::vector<cv::Point2f> reproj_points;
		cv::projectPoints(objp.at(i), rvecs.at(i), tvecs.at(i), result.cam_Kk.K, dist_coeffs, reproj_points);
		singular_error.at(i) = cv::norm(imgp.at(i), reproj_points, cv::NORM_L2) / imgp.at(i).size();
		});
	result.reproj_error = std::accumulate(singular_error.begin(), singular_error.end(), 0.0) / singular_error.size();
	result.success = true;
	return result;
}

const cv::Mat generate_board_image(const int board_width, const int board_height)
{
	const int img_width = (board_width >= 2) ? board_width + 1 : 3;
	const int img_height = (board_height >= 2) ? board_height + 1 : 3;
	cv::Mat out_img(img_height, img_width, CV_8UC1);
	for (int r = 0; r < img_height; ++r) {
		for (int c = 0; c < img_width; ++c) {
			out_img.at<uchar>(r, c) = ((r + c) % 2 == 0) ? 0 : 255;
		}
	}
	return out_img;

}
