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
#include "segmentation.h" 		 

std::vector<int64_t>      INPUT_SHAPE = {1, 3, 512, 512};
static std::vector<float> INPUT_MEAN  = {0.5f, 0.5f, 0.5f};
static std::vector<float> INPUT_STD   = {0.5f, 0.5f, 0.5f};
static int CLASS_NUM = 3;

paddle::lite_api::MobileConfig seg_config;
std::shared_ptr<paddle::lite_api::PaddlePredictor> seg_predictor;

void seg_init(std::string model_dir)
{
	printf("segmentation init.\n");
	seg_config.set_model_dir(model_dir); // model dir
	seg_config.set_threads(1);
	seg_config.set_power_mode(paddle::lite_api::LITE_POWER_HIGH); // power mode
	seg_predictor = paddle::lite_api::CreatePaddlePredictor<paddle::lite_api::MobileConfig>(seg_config); 
	return;
}

/*Preprocessing : resize and normalization*/
void seg_preprocess(std::vector<cv::Mat> &input_image, 
                    const std::vector<float> &input_mean,
                    const std::vector<float> &input_std, 
                    int input_width,
                    int input_height, 
                    float *input_data)
{
	int num = input_image.size();

	for (int i_num=0; i_num<num; i_num++)
	{
		cv::Mat resize_image;
		cv::Size resize_size(INPUT_SHAPE[2], INPUT_SHAPE[3]);
		cv::resize(input_image[i_num], resize_image, resize_size, 0, 0, cv::INTER_LINEAR);

		int channels = resize_image.channels();
		int rw = resize_image.cols;
		int rh = resize_image.rows;
		float *data = input_data + i_num * (channels * rw * rh);
//		printf("resize image : [channels:%d, w:%d, h:%d]\n", channels, rw, rh);	
		for (int h = 0; h < rh; ++h)
		{
			const unsigned char* uptr = resize_image.ptr<unsigned char>(h);
			const float* fptr = resize_image.ptr<float>(h);
			int im_index = 0;
			for (int w = 0; w < rw; ++w)
			{
				for (int c = 0; c < channels; ++c)
				{
					int top_index = (c * rh + h) * rw + w;
					float pixel;
					pixel = static_cast<float>(uptr[im_index++]) / 255.0;
					pixel = (pixel - input_mean[c]) / input_std[c];
					data[top_index] = pixel;
				}
			}
		}		
	}

	return;
}

/*Postprocessing : get the the bounding box result and creat output image*/
void seg_postprocess(float* p_out, int length, std::vector<std::vector<unsigned char>> &output) 
{
    int eval_width = INPUT_SHAPE[3];
    int eval_height = INPUT_SHAPE[2];
    int eval_num_class = CLASS_NUM;

    int blob_out_len = length;
    int seg_out_len = eval_height * eval_width * eval_num_class;
	int num = length/seg_out_len;

    if (length%seg_out_len)
	{
		printf("output length error, %d\n", length%seg_out_len);
        return;
    }

//	printf("------------------------------------detecter_postprocess 1\n");
	for (int i_num=0; i_num<num; i_num++)
	{
		std::vector<unsigned char> temp;
//		temp.resize(eval_height * eval_width);
//		temp.clear();
		output.push_back(temp);
		int out_img_len = eval_height * eval_width;
		float* data = &(p_out[i_num * seg_out_len]);
		for (int i = 0; i < out_img_len; ++i)
		{
			float max_value = -1;
			int label = 0;
			for (int j = 0; j < eval_num_class; ++j)
			{
				int index = i + j * out_img_len;
				if (index >= blob_out_len)
				{
					break;
				}
				float value = data[index];
				if (value > max_value)
				{
					max_value = value;
					label = j;
				}
			}
			output[i_num].push_back((unsigned char)(label*100));
		}
	}
    //post process

//	printf("------------------------------------detecter_postprocess 2\n");

	if(debug_level>=MONITOR){
		for (int i_num=0; i_num<num; i_num++)
		{
			cv::Mat meter_png = cv::Mat(eval_height, eval_width, CV_8UC1);
			meter_png.data = output[i_num].data();
			cv::imshow("SEG", meter_png);
			cv::waitKey(0);
		}
	}
	
    return;
}

void seg_process(std::vector<cv::Mat> &input_image, std::vector<std::vector<unsigned char>> &output) 
{
	int meter_num = input_image.size();
//	printf("------------------------------------process 1\n");
	// Preprocess image and fill the data of input tensor
	std::unique_ptr<paddle::lite_api::Tensor> input_tensor(std::move(seg_predictor->GetInput(0)));
	INPUT_SHAPE[0] = (int64_t)(meter_num);
	input_tensor->Resize(INPUT_SHAPE);
	auto *input_data = input_tensor->mutable_data<float>();
	int input_width = INPUT_SHAPE[3];
	int input_height = INPUT_SHAPE[2];
//	printf("------------------------------------process 2\n");
	printf("segmentation preprocess.\n");
	seg_preprocess(input_image, INPUT_MEAN, INPUT_STD, input_width, input_height, input_data);
//	printf("------------------------------------process 3\n");

		
	// actual products
	printf("segmentation run ...\n");
    seg_predictor->Run();
//	printf("------------------------------------process 4\n");

	// Get the data of output tensor and postprocess to output detected objects
	printf("segmentation postprocess.\n");
	std::unique_ptr<const paddle::lite_api::Tensor> output_tensor(std::move(seg_predictor->GetOutput(0)));
	float *output_data = output_tensor->mutable_data<float>();
	int64_t output_size = 1;
	for (auto dim : output_tensor->shape())
	{
		output_size *= dim;
//		printf("------------------------------------output_size:%ld\n", output_size);
	}
//	printf("------------------------------------process 5\n");
	seg_postprocess(output_data, output_size, output);
//	printf("------------------------------------process 6\n");

	return;
}

