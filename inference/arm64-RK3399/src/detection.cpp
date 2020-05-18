#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <limits>
#include <sys/time.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>

#include "paddle_api.h" 	

#include "global.h" 	
#include "detection.h" 		 

extern std::vector<int> IMAGE_SHAPE;

static std::vector<int64_t> INPUT_SHAPE = {1, 3, 608, 608};
static std::vector<float>   INPUT_MEAN  = {0.485f, 0.456f, 0.406f};
static std::vector<float>   INPUT_STD   = {0.229f, 0.224f, 0.225f};
static float SCORE_THRESHOLD = 0.5f;

#define CLASS_ID 74


using namespace paddle::lite_api;

MobileConfig config;
std::shared_ptr<PaddlePredictor> predictor;

void model_init(std::string model_dir)
{
	config.set_model_dir(model_dir);
	config.set_threads(1);
	config.set_power_mode(LITE_POWER_HIGH);
	predictor = CreatePaddlePredictor<MobileConfig>(config); 
	return;
}

void preprocess(cv::Mat &input_image, 
                const std::vector<float> &input_mean,
                const std::vector<float> &input_std, 
                int width, int height, float *output_data)
{
	cv::Mat im_temp;
	cv::Size resize_size(width, height);
	cv::resize(input_image, im_temp, resize_size, 0, 0, cv::INTER_LINEAR);

	int channels = im_temp.channels();
	int rw = im_temp.cols;
	int rh = im_temp.rows;
	for (int h = 0; h < rh; ++h) {
		const unsigned char* uptr = im_temp.ptr<unsigned char>(h);
		const float* fptr = im_temp.ptr<float>(h);
		int im_index = 0;
		for (int w = 0; w < rw; ++w) {
			for (int c = 0; c < channels; ++c) {
				int top_index = (c * rh + h) * rw + w;
				float pixel;
				pixel = static_cast<float>(uptr[im_index++]) / 255.0;
				pixel = (pixel - input_mean[c]) / input_std[c];
				output_data[top_index] = pixel;
			}
		}
	}
	return;
}

/*Postprocessing : get the the bounding box result and creat output image*/
std::vector<DETECTION_RESULT> detecter_postprocess(const float *output_data,
                                                   int64_t output_size,
                                                   const float score_threshold)
{
	std::vector<DETECTION_RESULT> results;

	for (int64_t i = 0; i < output_size; i += 6)
	{
		if (output_data[i + 1] < score_threshold)
		{
			continue;
		}
		if ((int)(output_data[i]) != CLASS_ID)
		{
			continue;
		}

		DETECTION_RESULT result;
		result.classid = output_data[i];
		result.score = output_data[i + 1];
		result.left = output_data[i + 2];
		result.top = output_data[i + 3];
		result.right = output_data[i + 4];
		result.bottom = output_data[i + 5];
		
		printf("get a bounding box : [classid:%f, score:%f, %f, %f, %f, %f]\n", result.classid, result.score, result.left, result.top, result.right, result.bottom);

		results.push_back(result);
	}

	return results;
}

void detecter_process(cv::Mat &input_image, std::vector<DETECTION_RESULT> &output) 
{
	std::unique_ptr<Tensor> input_tensor(std::move(predictor->GetInput(0)));
	input_tensor->Resize({1,3,608,608});
	auto *input_data_1 = input_tensor->mutable_data<float>();
	
	std::unique_ptr<Tensor> input_tensor_2(std::move(predictor->GetInput(1)));
	input_tensor_2->Resize({1, 2});
	auto *input_data_2 = input_tensor_2->mutable_data<int>();


	// Preprocess image and fill the data of input tensor
	printf("detecter preprocess.\n");
	detecter_preprocess(input_image, INPUT_MEAN, INPUT_STD, input_width, input_height, input_data);
	
	// actual products
	printf("detecter run ...\n");
	
    predictor->Run();

	// Get the data of output tensor and postprocess to output detected objects
	printf("detecter postprocess.\n");
	
	std::unique_ptr<Tensor> output_tensor(std::move(predictor->GetOutput(0)));
	float *output_data = output_tensor->mutable_data<float>();
	
	int64_t output_size = 1;
	for (auto dim : output_tensor->shape())
	{
		output_size *= dim;
	}
	output = detecter_postprocess(output_data, output_size, SCORE_THRESHOLD);

	return;
}


