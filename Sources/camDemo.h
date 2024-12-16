/*****************************************************************
 *	File...:	camDemo.h
 *	Purpose:	video processing
 *  Author.:	
 *	Date...:	30.09.2019
 *	Changes:	
 ****************************************************************/
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp> 
#include <opencv2/highgui/highgui_c.h>


#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "stdafx.h"
#include <time.h>

#pragma comment( lib, "winmm.lib")

/* add the libs to the list of library dependencies */
#ifdef _DEBUG	
	#pragma comment(lib, "../bin_debug/opencv_core401d.lib")
	#pragma comment(lib, "../bin_debug/opencv_highgui401d.lib")
	#pragma comment(lib, "../bin_debug/opencv_imgproc401d.lib")
	#pragma comment(lib, "../bin_debug/opencv_imgcodecs401d.lib") 
	#pragma comment(lib, "../bin_debug/opencv_videoio401d.lib")		
	#pragma comment(lib, "../bin_debug/opencv_objdetect401d.lib")
#else	
	#pragma comment(lib, "../bin/opencv_core401.lib")
	#pragma comment(lib, "../bin/opencv_highgui401.lib")
	#pragma comment(lib, "../bin/opencv_imgproc401.lib")   
	#pragma comment(lib, "../bin/opencv_imgcodecs401.lib")   
	//#pragma comment(lib, "../bin/opencv_video401.lib")	
	#pragma comment(lib, "../bin/opencv_videoio401.lib")	
	#pragma comment(lib, "../bin/opencv_objdetect401.lib")	
#endif

using namespace std;
using namespace cv;



#define median(a,b,c)  ((c) > (max(a,b)) ? max(a,b) : (c) < min(a,b) ? min(a,b) : (c) )

#ifndef ALLOC
#define ALLOC(_ptr, _num, _type)									\
{																									\
	_ptr = (_type *)malloc((_num) * sizeof(_type));	\
	if (_ptr == NULL)								\
	{																\
		fprintf(stderr, "malloc failed for %s %s\n", #_num, #_type);	\
	}																\
}
#endif

/*	resolution of the internal image that is used for all image processing steps	*/



typedef __int64 TDateTime;  // This is actually an alias to NT FILETIME.
inline TDateTime  CurrDateTime()
{
      TDateTime param;
      GetSystemTimeAsFileTime((FILETIME*)&param);
      return(param);
}
#define   ONE_SECOND    ((__int64)10000000)



typedef enum
{
	START_SCREEN,
	DEMO_ACTIVE,
	DEMO_FINISHED,
	DEMO_STOP
} DemoState;


struct MouseParams
{
    Point2i mouse_pos;
    int evt;
};

void createBlackHoleEffect(cv::Mat& inputImage, int centreX, int centreY, int radius, int radiusMult, float scalingFactor, int marginWidth);
