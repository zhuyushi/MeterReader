#ifndef _READVALUE_H_
#define _READVALUE_H_	 

struct READ_RESULT {
	int   scale_num;
	float scales;
	float ratio;
};

void creat_line_image(std::vector<unsigned char> &seg_image, std::vector<unsigned char> &output);

void convert_1D_data(std::vector<unsigned char> &line_image, std::vector<unsigned int> &scale_data, std::vector<unsigned int> &pointer_data);

void read_process(std::vector<std::vector<unsigned char>> &seg_image, std::vector<READ_RESULT> &read_results);

#endif

