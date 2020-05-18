#ifndef _SEGMENTATION_H_
#define _SEGMENTATION_H_	 

void seg_init(std::string model_dir);

void seg_preprocess(std::vector<cv::Mat> &input_image, 
                    const std::vector<float> &input_mean,
                    const std::vector<float> &input_std, 
                    int input_width,
                    int input_height, 
                    float *input_data);

void seg_postprocess(float* p_out, int length, std::vector<std::vector<unsigned char>> &output);

void seg_process(std::vector<cv::Mat> &input_image, std::vector<std::vector<unsigned char>> &output);

#endif

