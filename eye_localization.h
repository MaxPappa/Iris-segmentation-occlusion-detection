#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <stdio.h>
#include <iostream>


using namespace cv;
using namespace std;


/**
 * @file
 * @brief localizzazione della zona perioculare
 */

float obtain_radii(float ki, float c_length);
void auto_canny(Mat* input, Mat* output, float sigma=0.33);
bool isVertical(string imgPath);
Mat threshSkinImg(Mat img);
float* clip(float min, float max, float* values, int v_size);
bool shouldUseVJ(string imgPath);