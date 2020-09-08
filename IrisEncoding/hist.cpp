#include "hist.hpp"

void computeHist(cv::Mat img_in, cv::Mat histogram) {
	int histSize = 256; //from 0 to 255
	float range[] = { 0, 256 }; //the upper boundary is exclusive
	const float* histRange = { range };
	cv::calcHist(&img_in, 1, 0, cv::Mat(), histogram, 1, &histSize, &histRange);
}