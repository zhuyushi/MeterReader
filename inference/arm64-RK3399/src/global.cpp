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

std::vector<int> IMAGE_SHAPE = {1920, 1080};
std::vector<int> RESULT_SHAPE = {1280, 720};
unsigned char debug_level = 0;

#define METER_TYPE_NUM 2
MeterConfig_T meter_config[METER_TYPE_NUM] = {
{10.0f/50.0f, 10.0f,  "(MPa)"},
{1.6f/32.0f,  1.6f,   "(MPa)"}
};

int64_t get_current_us()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return 1000000LL * (int64_t)time.tv_sec + (int64_t)time.tv_usec;
}

