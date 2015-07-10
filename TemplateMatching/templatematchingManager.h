#pragma once

#include <opencv2\opencv.hpp>

#define DLLExport __declspec (dllexport)


using namespace cv;

struct TemplateLabel
{
	int templateNum;		// テンプレート番号(0は割り振られていない)
	int checkNum;			// 発見回数
	bool detect_flag;		// 検出確認(5回連続で検出されたらtrue)
	int objectID;			// オブジェクト毎に割り振るID

	// 発見が1回以上
	int detect_x;			// 発見したx座標
	int detect_y;			// 発見したy座標

	// 検出時
	cv::Mat depthRect;		// 検出時の深度値
	
	
	TemplateLabel()
		: templateNum (0)
		, checkNum (0)
		, detect_flag (false)
		, objectID (-1)
		, detect_x (0)
		, detect_y (0)
	{}
};

extern "C" {
	DLLExport void templateMatchingManager(unsigned char* const ir_data, unsigned short* const depth_data, TemplateLabel Label[], int labelNum, int desk_pos_y, int desk_pos_z);

	DLLExport void openWindow(const char* windowName);

	DLLExport void closeWindow(const char* windowName);

}