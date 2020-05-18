#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <limits>
#include <sys/time.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>

#include "global.h" 	
#include "detection.h"
#include "segmentation.h"
#include "readvalue.h" 	

void process(cv::Mat &input_image)
{
	if(debug_level>=MONITOR){
		cv::Mat input_show;
		cv::Size show_size(RESULT_SHAPE[0], RESULT_SHAPE[1]);
		cv::resize(input_image, input_show, show_size, 0, 0, cv::INTER_LINEAR);
		cv::imshow("METER", input_show);
		cv::waitKey(0);
	}
	
	double start_time = get_current_us();

	std::vector<DETECTION_RESULT> detection_results;
	detection_results.clear();
	printf("meter detecter start.\n");
	detecter_process(input_image, detection_results);

	int meter_num = detection_results.size();
	if(!meter_num)
	{
		printf("don't get meter.\n");
		return;
	}
	else
	{
		printf("get %d meter.\n", meter_num);
	}

	std::vector<cv::Mat> meters_image;
	for(int i_meter=0; i_meter<meter_num; i_meter++)
	{
		int left = static_cast<int>(detection_results[i_meter].left);
		int top = static_cast<int>(detection_results[i_meter].top);
		int right = static_cast<int>(detection_results[i_meter].right);
		int bottom = static_cast<int>(detection_results[i_meter].bottom);

		cv::Mat sub_image = input_image(cv::Range(top, bottom+1), cv::Range(left, right+1));
		meters_image.push_back(sub_image);
		if(debug_level>=MONITOR){
			cv::imshow("METER", meters_image[i_meter]);
			cv::waitKey(0);
		}
	}

	std::vector<std::vector<unsigned char>> seg_reault;
	seg_reault.clear();
	printf("meter segmentation start.\n");
	seg_process(meters_image, seg_reault);

	std::vector<READ_RESULT> read_results;
	read_results.clear();
	printf("meter read start.\n");
	read_process(seg_reault, read_results);

	double end_time = get_current_us();
	double process_time = (end_time - start_time) / 1000.0f / 1000.0f;
	
	cv::Mat output_image = input_image.clone();
	for(int i_results=0; i_results<read_results.size(); i_results++)
	{
		float result = 0;;
		std::string unit_str;
		if(read_results[i_results].scale_num>TYPE_THRESHOLD)
		{
			result = read_results[i_results].scales * meter_config[0].scale_value;
//			result = read_results[i_results].ratio * meter_config[0].range;
			unit_str = meter_config[0].str;
		}
		else
		{
			result = (read_results[i_results].scales-1) * meter_config[1].scale_value;
//			result = read_results[i_results].ratio * meter_config[0].range;
			unit_str = meter_config[1].str;
		}
		printf("meter %d: scale_num:%d, scales:%f, ratio:%f, unit:%f.\n", i_results, read_results[i_results].scale_num, read_results[i_results].scales, read_results[i_results].ratio, meter_config[0].scale_value);
		printf("read result %d : %f.\n", i_results, result);

		int lx = static_cast<int>(detection_results[i_results].left);
		int ly = static_cast<int>(detection_results[i_results].top);
		int w = static_cast<int>(detection_results[i_results].right) - lx;
		int h = static_cast<int>(detection_results[i_results].bottom) - ly;

		cv::Rect bounding_box = cv::Rect(lx, ly, w, h) & cv::Rect(0, 0, output_image.cols, output_image.rows);
		if (w > 0 && h > 0)
		{
			cv::Scalar color = cv::Scalar(237, 189, 101);
			cv::rectangle(output_image, bounding_box, color);
			cv::rectangle(output_image, cv::Point2d(lx, ly), cv::Point2d(lx + w, ly - 30), color, -1);
			
			std::string class_name = "Meter";
			cv::putText(output_image,
						class_name + " : " + std::to_string(result) + unit_str,
						cv::Point2d(lx, ly-5), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
		}
	}
	
	printf("Process time: %f s\n", process_time);
	
	cv::Mat result_image;
	cv::Size resize_size(RESULT_SHAPE[0], RESULT_SHAPE[1]);
	cv::resize(output_image, result_image, resize_size, 0, 0, cv::INTER_LINEAR);
	cv::imshow("METER", result_image);
	cv::waitKey(0);

	return;
}


/*
meter /home/firefly/Meter/model/yolov3_darknet_arm /home/firefly/Meter/model/deeplabv3p_xception65_arm <debug|monitor|result> <10|<input_image> <output_image>>
*/
int main(int argc, char **argv)
{
	printf("meter start\n");

	if (argc != 5 && argc != 6)
	{
		printf(
			"Usage: \n"
			"./meter <detection_model_dir> <seg_model_dir> <device_num|(input_image_path output_image_path)>\n"
			"use images from camera if input_image_path and input_image_path isn't provided.\n");
		return -1;
	}

	std::string detection_model_dir = argv[1];
	std::string seg_model_dir = argv[2];
	std::string model_str = argv[3];
	if (model_str == "monitor")
	{
		debug_level = MONITOR;
	}
	else if (model_str == "debug")
	{
		debug_level = DEBUG;
	}

	printf("detection_model_dir:%s\n", detection_model_dir.c_str());
	printf("seg_model_dir:%s\n", seg_model_dir.c_str());
	/* PaddleLite Init */
	detecter_init(detection_model_dir);
	seg_init(seg_model_dir);

	/* use camera */
	if (argc == 5)
	{
		int devicenum = atoi(argv[4]);
		
		cv::VideoCapture cap(devicenum);
		cap.set(CV_CAP_PROP_FRAME_WIDTH, IMAGE_SHAPE[0]);
		cap.set(CV_CAP_PROP_FRAME_HEIGHT, IMAGE_SHAPE[1]);
		if (!cap.isOpened()) 
		{
			printf("open camera failed.\n");
			return -1;
		}
		printf("open camera succ\n");

		while (1) 
		{
			cv::Mat input_image;
			cap >> input_image;
			printf("-------------------------\n");
			printf("got camera image.\n");

			process(input_image);
		}
		cap.release();
		cv::destroyAllWindows();
	}
	/* use image */
	else if (argc == 6)
	{
		std::string input_image_path = argv[4];
		std::string output_image_path = argv[5];
		
		cv::Mat input_image = cv::imread(input_image_path);

		process(input_image);
	}

	printf("meter end\n");
	return 0;
}

