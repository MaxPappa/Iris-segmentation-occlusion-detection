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
    cv::Mat imgInp; 
    cv::inpaint(*(eye->getEyeImgRes()), *(eye->getMask()), imgInp, iterations, cv::INPAINT_TELEA);
    eye->setImgInp(&imgInp);
    cv::Mat bgr[3];
    cv::split(imgInp, bgr);
    eye->setBlueInp(&bgr[0]);
    eye->setGreenInp(&bgr[1]);
    eye->setRedInp(&bgr[2]);
}

void Preprocessing::run()
{
    auto[width, height] = obtain_w_h(eye->getEyeImg()->cols, eye->getEyeImg()->rows);
    eye->resize(width, height);
    int ksize = 3; double c = -20;
    searchReflection(eye, ksize, c);
    inpaintReflection(eye, 1);
}