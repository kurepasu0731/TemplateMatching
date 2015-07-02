#pragma once

#include <opencv2/opencv.hpp>

#define DLLExport __declspec (dllexport)


using namespace cv;


extern "C" {
	DLLExport void templateMatching(std::string tmp_img_filename, std::string search_img_filename, double thresh, int& x, int& y);
}