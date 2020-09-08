#include "Matcher.hpp"
#include <iostream>


Matcher::Matcher(FeatExtractor method)
{
    this->method = method;
}

Matcher::~Matcher()
{
}

double Matcher::match(Subject* sub1, Subject* sub2)
{
    switch (this->method)
    {
        case LBP_M:
            return match_LBP(sub1,sub2);
            break;
        
        default:
            break;
    }
}


double Matcher::match_LBP(Subject* sub1, Subject* sub2)
{
    int subRows = sub1->encodedImg.rows / NUM_SLICES;
	cv::Mat normHist1 = cv::Mat::zeros(256, 1, CV_32FC1);
	cv::Mat normHist2 = cv::Mat::zeros(256, 1, CV_32FC1);
	cv::Mat mask1 = cv::Mat::zeros(subRows, sub1->encodedImg.cols, CV_8UC1);
	cv::Mat mask2 = cv::Mat::zeros(subRows, sub2->encodedImg.cols, CV_8UC1);

	double distance = 0.0;

    cv::Mat histograms1[NUM_SLICES];
	cv::Mat histograms2[NUM_SLICES];

	int histSize = 256; //from 0 to 255
	float range[] = { 0, 256 }; //the upper boundary is exclusive
	const float* histRange = { range };
	
	for (int i = 0; i < NUM_SLICES; i++) {
		
		cv::calcHist(&sub1->encodedImg(cv::Range(subRows*i, subRows*(i + 1)), cv::Range(0, sub1->encodedImg.cols)), 1, 0, cv::Mat(), histograms1[i], 1, &histSize, &histRange);
		cv::calcHist(&sub2->encodedImg(cv::Range(subRows*i, subRows*(i + 1)), cv::Range(0, sub2->encodedImg.cols)), 1, 0, cv::Mat(), histograms2[i], 1, &histSize, &histRange);

		cv::normalize(histograms1[i], normHist1);
		cv::normalize(histograms2[i], normHist2);

		sub1->mask(cv::Range(subRows*i, subRows*(i + 1)), cv::Range(0, sub1->encodedImg.cols)).copyTo(mask1);
		sub2->mask(cv::Range(subRows*i, subRows*(i + 1)), cv::Range(0, sub2->encodedImg.cols)).copyTo(mask2);

		std::cout << "round num " << i << ": " << cv::compareHist(normHist1, normHist2, CV_COMP_BHATTACHARYYA) << std::endl;
		distance += cv::compareHist(normHist1, normHist2, CV_COMP_BHATTACHARYYA)
			* (1.0 - (((2*mask1.rows*mask1.cols) - (countNonZero(mask1) + countNonZero(mask2))) 
				/ (double)(2 * mask1.rows*mask1.cols)));
		std::cout << "tmp dist is = " << distance << std::endl;
	}
	std::cout << distance / NUM_SLICES << std::endl;
	return distance / NUM_SLICES;
}