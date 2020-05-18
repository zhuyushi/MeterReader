#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <limits>
#include <sys/time.h>
#include <cmath>
#include <cstdio>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>

#include "paddle_api.h" 	

#include "global.h" 	
#include "readvalue.h" 	

#define SEG_IMAGE_SIZE 512
#define LINE_HEIGH 120
#define LINE_WIDTH 1570
#define CIRCLE_RADIUS 250

const float pi = 3.1415926536f;
const int circle_center[] = {256, 256};

   
static void vector2mat(std::vector<unsigned int> &a, const char *str)
{
	cv::Mat M(LINE_HEIGH,LINE_WIDTH,CV_8UC1);
	for (int i = 0; i < M.cols; ++i)
	{
		unsigned int tmp = a[i];
		for (int j = 0; j < M.rows; ++j)
		{
			unsigned char *p = M.ptr<unsigned char>(j);
			if(tmp>0)
			{
				p[i] = 250;
				tmp--;
			}
			else
			{
				p[i] = 0;
			}
		}
	}
	cv::imshow(str, M);
	cv::waitKey(0);
	return;
}

void creat_line_image(std::vector<unsigned char> &seg_image, std::vector<unsigned char> &output)
{
	float theta;
	int rho;
	int image_x;
	int image_y;
	
	output.resize(LINE_HEIGH*LINE_WIDTH);
	output.clear();
	for(int row=0; row<LINE_HEIGH; row++)
	{
		for(int col=0; col<LINE_WIDTH; col++)
		{
            theta = pi*2/LINE_WIDTH*(col+1);
			rho = CIRCLE_RADIUS-row-1;
            image_x = int(circle_center[0] + rho*cos(theta)+0.5);
            image_y = int(circle_center[1] - rho*sin(theta)+0.5);          
            if(seg_image[image_x*SEG_IMAGE_SIZE+image_y] == 100)
        	{
//                output[(LINE_WIDTH-row)*LINE_HEIGH + LINE_HEIGH-col] = 100;
                output[row*LINE_WIDTH + col] = 100;
        	}
			else if(seg_image[image_x*SEG_IMAGE_SIZE+image_y] == 200)
			{
//                output[(LINE_WIDTH-row)*LINE_HEIGH + LINE_HEIGH-col] = 200;
                output[row*LINE_WIDTH + col] = 200;
			}
		}
	}

	return;
}

void convert_1D_data(std::vector<unsigned char> &line_image, std::vector<unsigned int> &scale_data, std::vector<unsigned int> &pointer_data)
{
	for(int col=0; col<LINE_WIDTH; col++)
	{
		scale_data[col] = 0;
		pointer_data[col] = 0;
		for(int row=0; row<LINE_HEIGH; row++)
		{
			if(line_image[row*LINE_WIDTH + col] == 100)
			{
				pointer_data[col]++;
			}
			else if(line_image[row*LINE_WIDTH + col] == 200)
			{
				scale_data[col]++;
			}
		}
	}
	return;
}

void scale_mean_filtration(std::vector<unsigned int> &scale_data, std::vector<unsigned int> &scale_mean_data)
{
	int sum = 0;
	int mean = 0;
	int size = scale_data.size();
	for(int i=0; i<size; i++)
	{
		sum = sum + scale_data[i];
	}
	mean = sum/size;

	for(int i=0; i<size; i++)
	{
		if(scale_data[i]>=mean)
		{
			scale_mean_data[i] = scale_data[i];
		}
	}
	
	return;
}

READ_RESULT get_meter_reader(std::vector<unsigned int> &scale, std::vector<unsigned int> &pointer)
{
	std::vector<unsigned int> scale_location;
	unsigned int one_scale_location = 0;
	unsigned char scale_flag = 0;
	unsigned int one_scale_start = 0;
	unsigned int one_scale_end = 0;

	unsigned int pointer_location = 0;
	unsigned char pointer_flag = 0;
	unsigned int one_pointer_start = 0;
	unsigned int one_pointer_end = 0;

	READ_RESULT result;
	
	for (int i=0; i<LINE_WIDTH; i++)
	{
		//scale location
		if ((scale[i]>0) && (scale[i+1]>0))
		{
			if (scale_flag == 0)
			{
				one_scale_start = i;
				scale_flag = 1;
			}
		}
		if (scale_flag == 1)
		{
			if ((scale[i]==0) && (scale[i+1]==0))
			{
				one_scale_end = i - 1;
				one_scale_location = (one_scale_start + one_scale_end)/2;
				scale_location.push_back(one_scale_location);
				one_scale_start = 0;
				one_scale_end = 0;
				scale_flag = 0;
			}
		}

		//pointer location
		if ((pointer[i]>0) && (pointer[i+1]>0))
		{
			if (pointer_flag == 0)
			{
				one_pointer_start = i;
				pointer_flag = 1;
			}
		}
		if (pointer_flag == 1)
		{
			if ((pointer[i]==0) && (pointer[i+1]==0))
			{
				one_pointer_end = i - 1;
				pointer_location = (one_pointer_start + one_pointer_end)/2;
				one_pointer_start = 0;
				one_pointer_end = 0;
				pointer_flag = 0;
			}
		}
	}

	if(debug_level>=DEBUG){
		std::vector<unsigned int> temp(LINE_WIDTH, 0);
		temp.clear();
		int scale_location_size = scale_location.size();
		for(int i=0; i<scale_location_size; i++)
		{
			temp[scale_location[i]] = scale[scale_location[i]];
		}
		temp[pointer_location] = pointer[pointer_location];
		vector2mat(temp, "location");
	}

	int scale_num = scale_location.size();
	result.scale_num = scale_num;
	for(int i=0; i<scale_num-1; i++)
	{
		if ((scale_location[i]<=pointer_location) && (pointer_location<scale_location[i+1]))
		{
			result.scales = ((float)(i+1)) + ((float)(pointer_location-scale_location[i]))/((float)(scale_location[i+1]-scale_location[i]));
		}
	}
	result.ratio = ((float)(pointer_location-scale_location[0]))/((float)(scale_location[scale_num-1]-scale_location[0]));
	
	printf("meter read result : scale_num:%d, value:%f, ratio:%f.\n", result.scale_num, result.scales, result.ratio);

	return result;
}

void read_process(std::vector<std::vector<unsigned char>> &seg_image, std::vector<READ_RESULT> &read_results)
{
	int read_num = seg_image.size();
	printf("meter read read_num : %d.\n", read_num);
	for(int i_read=0; i_read<read_num; i_read++)
	{
		std::vector<unsigned char> line_result;
		creat_line_image(seg_image[i_read], line_result);

		if(debug_level>=DEBUG){
			cv::Mat read_png = cv::Mat(LINE_HEIGH, LINE_WIDTH, CV_8UC1);
			read_png.data = line_result.data();
			cv::imshow("READ", read_png);
			cv::waitKey(0);
		}

		std::vector<unsigned int> scale_data(LINE_WIDTH);
		std::vector<unsigned int> pointer_data(LINE_WIDTH);
		convert_1D_data(line_result, scale_data, pointer_data);
		if(debug_level>=DEBUG){
			vector2mat(scale_data, "scale");
			vector2mat(pointer_data, "pointer");
		}

		std::vector<unsigned int> scale_mean_data(LINE_WIDTH);
		scale_mean_filtration(scale_data, scale_mean_data);
		if(debug_level>=DEBUG){
			vector2mat(scale_mean_data, "scale_mean");
		}

		READ_RESULT result = get_meter_reader(scale_mean_data, pointer_data);
		read_results.push_back(result);
	}

	return;
}

