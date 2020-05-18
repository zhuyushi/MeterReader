#ifndef _METER_H_
#define _METER_H_	 

enum Level 
{
	RESULT,
	MONITOR, 
	DEBUG
};

typedef struct MeterConfig{
	float scale_value;
	float range;
	char  str[10];
}MeterConfig_T;

extern std::vector<int> IMAGE_SHAPE;
extern std::vector<int> RESULT_SHAPE;
extern unsigned char debug_level;
extern MeterConfig_T meter_config[];

#define TYPE_THRESHOLD 40

int64_t get_current_us();

#endif

