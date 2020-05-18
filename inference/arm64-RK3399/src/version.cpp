
#ifdef VERSION
	char const version_string[] = VERSION;
#else
	char const version_string[] = "Unknow";
#endif

#ifdef C_DATE
	char const date_string[]=C_DATE;
#else
	char const date_string[]="Unknow";
#endif

#ifdef C_TIME
	char const time_string[]=C_TIME;
#else
	char const time_string[]="Unknow";
#endif

