#include "templatematching.h"

DLLExport void templateMatching(std::string tmp_img_filename, std::string search_img_filename, double thresh, int& x, int& y)
{
	// 探索画像
	cv::Mat search_img = cv::imread(search_img_filename, 1);
		 
	// テンプレート画像
	cv::Mat tmp_img = cv::imread(tmp_img_filename, 1);
		 
	cv::Mat result_img;
	// テンプレートマッチング
	cv::matchTemplate(search_img, tmp_img, result_img, CV_TM_CCOEFF_NORMED);

	// 最大のスコアの場所を探す
	//cv::Rect roi_rect(0, 0, tmp_img.cols, tmp_img.rows);
	cv::Point max_pt;
	double maxVal;
	cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);
	//roi_rect.x = max_pt.x;
	//roi_rect.y = max_pt.y;
	std::cout << "(" << max_pt.x << ", " << max_pt.y << "), score=" << maxVal << std::endl;
	// 探索結果の場所に矩形を描画
	if(maxVal > thresh){
		x = max_pt.x;
		y = max_pt.y;
	}else{
		//閾値以下ならエラー値-1を入れておく
		x = -1;
		y = -1;
	}
}
