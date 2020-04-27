#include "Preprocessing.hpp"

Preprocessing::Preprocessing(Eye* eye)
{
    this->eye = eye;
}

Preprocessing::~Preprocessing(){ eye = 0; }

void Preprocessing::searchReflection(Eye* eye, int ksize, double c)
{
    cv::Mat mask;
	cv::adaptiveThreshold(*(eye->getBlueSpec()), mask, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, ksize, c);
	dilate(mask, mask, cv::Mat(), cv::Point(-1,-1), 2);
    eye->setMask(&mask);
}

void Preprocessing::inpaintReflection(Eye* eye, int iterations)
{
    cv::inpaint(*(eye->getEyeImgRes()), *(eye->getMask()), *(eye->getImgInp()), iterations, cv::INPAINT_TELEA);
}

void Preprocessing::run()
{
    int ksize = 3; double c = -20;
    searchReflection(eye, ksize, c);
    inpaintReflection(eye, 1);
}