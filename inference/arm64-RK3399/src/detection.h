#ifndef _DETECTION_H_
#define _DETECTION_H_	 

struct DETECTION_RESULT {
  float classid;
  float score;
  float left;
  float top;
  float right;
  float bottom;
};

void detecter_init(std::string model_dir);

void detecter_preprocess(cv::Mat &input_image, 
                         const std::vector<float> &input_mean,
                         const std::vector<float> &input_std, 
                         int input_width,
                         int input_height, 
                         float *input_data);

std::vector<DETECTION_RESULT> detecter_postprocess(const float *output_data,
                                                   int64_t output_size,
                                                   const float score_threshold);


void detecter_process(cv::Mat &input_image, std::vector<DETECTION_RESULT> &output);

#endif

