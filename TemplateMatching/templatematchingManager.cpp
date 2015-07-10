#include "templatematchingManager.h"

DLLExport void templateMatchingManager(unsigned char* const ir_data, unsigned short* const depth_data, TemplateLabel Label[], int labelNum, int desk_pos_y, int desk_pos_z)
{
	/***** ⓪-1 IR,Depth画像復元 *****/
	int width = 512;
	int height = 424;

    cv::Mat input_image(height, width, CV_8UC4, ir_data);
    cv::Mat depth_image(height, width, CV_16UC1, depth_data);

	/***** ⓪-2 背景差分 *****/

	cv::Mat backImage( height, width, CV_16UC1 );
	cv::Mat mask( height, width, CV_8UC1 );
	cv::Mat diff;

	////読み込み用
	cv::FileStorage fs2("depthDATA.xml", cv::FileStorage::READ);
	fs2["depth_mat"] >> backImage;


	int noise = 50;

	for(int i=0;i<backImage.total();i++){
		//if( depthBuffer[i] <= backImage.at<UINT16>(i) - noise && depthBuffer[i] != 0){
		if( depth_data[i] <= backImage.at<unsigned short>(i) - noise && depth_data[i] != 0){
			mask.data[i]=255;
		}else{
			mask.data[i]=0;
		}
	}
	//マスクかける
	input_image.copyTo(diff, mask);
	
	diff.convertTo(diff, CV_8UC1, 255.0/65535.0);

	/***** ①マスク更新処理 *****/
	
	// テンプレート情報
	int allTemplateNum = 2;
	std::vector<std::pair<std::string, float>> fileNames;
	std::pair<std::string, float> fileName1;
	fileName1.first = "./Template/template1.png";
	fileName1.second = 0.65;
	fileNames.push_back(fileName1);
	std::pair<std::string, float> fileName2;
	fileName2.first = "./Template/template2.png";
	fileName2.second = 0.55;
	fileNames.push_back(fileName2);

	// テンプレート情報の格納
	std::vector<std::pair<cv::Mat, float>> templateImgs;
	for(int i = 0; i < allTemplateNum; ++i)
	{
		std::pair<cv::Mat, float> templateImg;
		templateImg.first = cv::imread(fileNames.at(i).first, 0);
		templateImg.second = fileNames.at(i).second;

		templateImgs.push_back(templateImg);
	}

	// 処理用の画像
	cv::Mat matching_img;
	//input_image.copyTo(matching_img);
	diff.copyTo(matching_img);

	// オブジェクトの下端の閾値
	std::vector<int> under_pos;
	//for(int i = 0; i < input_image.cols; ++i)
	for(int i = 0; i < diff.cols; ++i)
	{
		under_pos.push_back(desk_pos_y);		// 初期は机のy座標
	}

	// マスクの確認
	for(int i = 0; i < labelNum; ++i)
	{
		// 検出済みのラベルに対して
		if(Label[i].detect_flag)
		{
			
			// 前フレームからのズレ量の計算
			int sum_diff = 0;
			for(int x = Label[i].detect_x; x < Label[i].detect_x + templateImgs.at(Label[i].templateNum-1).first.cols; ++x)
			{
				for(int y = Label[i].detect_y; y < Label[i].detect_y + templateImgs.at(Label[i].templateNum-1).first.rows; ++y)
				{
					// 深度値が取れていない場合と机より手前の深度値は考慮しない
					if(depth_image.at<unsigned short>(y,x) != 0 && Label[i].depthRect.at<unsigned short>(y,x) != 0 && depth_image.at<unsigned short>(y,x) >= desk_pos_z)
					{
						sum_diff += abs((int)(Label[i].depthRect.at<unsigned short>(y,x) - depth_image.at<unsigned short>(y,x)) );
					}
				}				
			}

			// 全体の4分の1ずれたら検出を外す
			if(sum_diff >= templateImgs.at(Label[i].templateNum-1).first.cols * templateImgs.at(Label[i].templateNum-1).first.rows * 100 / 4 )
			{
				// ラベルの初期化
				Label[i].templateNum = 0;
				Label[i].checkNum = 0;
				Label[i].detect_flag = false;
				Label[i].objectID = -1;
				Label[i].detect_x = 0;
				Label[i].detect_y = 0;
			}
			else
			{
				// 検出済みのラベルはマスクする
				cv::Rect roi_rect(0, 0, templateImgs.at(Label[i].templateNum-1).first.cols, templateImgs.at(Label[i].templateNum-1).first.rows);
				roi_rect.x = Label[i].detect_x;
				roi_rect.y = Label[i].detect_y;

				cv::rectangle(matching_img, roi_rect, cv::Scalar(0), -1);

				// オブジェクトの下端の閾値の更新
				for(int x = Label[i].detect_x; x < Label[i].detect_x + templateImgs.at(Label[i].templateNum-1).first.cols; ++x)
				{
					if(Label[i].detect_y <= under_pos.at(x))
						under_pos.at(x) = Label[i].detect_y;
				}
			}
		}
	}


	/*****  ②マッチング処理 *****/
	cv::Mat result_img;
	cv::Point max_pt;
	double maxVal;
	std::vector<std::pair<int, cv::Point>> good_positions;

	for (int i = 0; i < templateImgs.size(); ++i)
	{
		cv::Rect roi_rect(0, 0, templateImgs.at(i).first.cols, templateImgs.at(i).first.rows);

		// 閾値以上のテンプレートを複数検出
		do
		{
			cv::matchTemplate(matching_img, templateImgs.at(i).first, result_img, CV_TM_CCOEFF_NORMED);		// テンプレートマッチング

			cv::minMaxLoc(result_img, NULL, &maxVal, NULL, &max_pt);

			// 閾値以上
			if(maxVal >= templateImgs.at(i).second)
			{
				// 一度発見した場所はマスクする
				roi_rect.x = max_pt.x;
				roi_rect.y = max_pt.y;
				cv::rectangle(matching_img, roi_rect, cv::Scalar(0), -1);

				// 積み上がっている付近の場合は仮登録
				int under_y = max_pt.y+templateImgs.at(i).first.rows;
				bool pile_flag = false;
				for(int x = max_pt.x; x < max_pt.x+templateImgs.at(i).first.cols; ++x)
				{
					if(under_y >= under_pos.at(x)-20 && under_y <= under_pos.at(x)+20)
					{
						pile_flag = true;
						break;
					}
				}
				if(pile_flag)
				{
					// 仮登録
					std::pair<int, cv::Point> good_position;
					good_position.first = i+1;
					good_position.second = max_pt;
					good_positions.push_back(good_position);
				}
			}
		}
		while(maxVal >= templateImgs.at(i).second);
	}


	/*****  ③ラベル登録処理 *****/

	// 既に検出済みのものや前回発見したもの以外は初期化
	for(int i = 0; i < labelNum; ++i)
	{
		bool init_flag = true;	// 初期化フラグ

		// 検出済みでないラベルに対して
		if(!Label[i].detect_flag)
		{
			// 前回発見した位置に同じオブジェクトがあるか検索
			for(int j = 0; j < good_positions.size(); ++j)
			{
				// テンプレート番号が同じで、座標位置が近い(近傍3画素以内)
				if( Label[i].templateNum == good_positions.at(j).first
					&& good_positions.at(j).second.x >= Label[i].detect_x-3 && good_positions.at(j).second.x <= Label[i].detect_x+3
					&& good_positions.at(j).second.y >= Label[i].detect_y-3 && good_positions.at(j).second.y <= Label[i].detect_y+3)
				{
					Label[i].checkNum++;	// 発見回数を増やす

					// 5回以上発見されたら検出
					if(Label[i].checkNum >= 5)
					{
						Label[i].detect_flag = true;
						Label[i].checkNum = 0;
						Label[i].depthRect = depth_image.clone();
					}

					good_positions.at(j).first = 0; // 登録フラグ

					// 発見したらループを抜ける
					init_flag = false;
					break;
				}
			}
		}
		else
		{
			init_flag = false;
		}

		// ラベルの初期化
		if(init_flag)
		{
			Label[i].templateNum = 0;
			Label[i].checkNum = 0;
			Label[i].detect_flag = false;
			Label[i].objectID = -1;
			Label[i].detect_x = 0;
			Label[i].detect_y = 0;
		}
	}

	// 新規に登録
	for(int i = 0; i < good_positions.size(); ++i)
	{
		// まだ登録されていなければ
		if(good_positions.at(i).first != 0)
		{
			for(int j = 0; j < labelNum; ++j )
			{
				// 登録されていないラベルであれば登録
				if(Label[j].templateNum == 0)
				{
					Label[j].templateNum = good_positions.at(i).first;
					Label[j].checkNum = 1;
					Label[j].detect_x = good_positions.at(i).second.x;
					Label[j].detect_y = good_positions.at(i).second.y;
				}
			}
		}
	}


	// 可視化(確認用)
	cv::cvtColor(matching_img, matching_img, CV_GRAY2BGR);

	cv::namedWindow("search image", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	cv::namedWindow("result image", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);

	for(int i = 0; i < labelNum; ++i)
	{
		std::cout << i << "番目:" << Label[i].templateNum << ", 回数:" << Label[i].checkNum << ", 座標;" << Label[i].detect_x << "," << Label[i].detect_y << std::endl;
		// 登録されているラベルのみ
		if(Label[i].templateNum != 0)
		{
			cv::Rect roi_rect(0, 0, templateImgs.at(Label[i].templateNum-1).first.cols, templateImgs.at(Label[i].templateNum-1).first.rows);
			roi_rect.x = Label[i].detect_x;
			roi_rect.y = Label[i].detect_y;

			if(Label[i].detect_flag)
				cv::rectangle(matching_img, roi_rect, cv::Scalar(0,0,255), 3);
			else if(Label[i].templateNum == 1)
				cv::rectangle(matching_img, roi_rect, cv::Scalar(0,255,0), 3);
			else
				cv::rectangle(matching_img, roi_rect, cv::Scalar(255,0,0), 3);
		}
	}

	//cv::imshow("search image", matching_img);
	//cv::imshow("result image", result_img);
}

DLLExport void openWindow(const char* windowName)
{
    cv::namedWindow(windowName, CV_WINDOW_NORMAL);
}

DLLExport void closeWindow(const char* windowName)
{
	cv::destroyWindow(windowName);
}

