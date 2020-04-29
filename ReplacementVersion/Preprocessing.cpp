#include "Preprocessing.hpp"

Preprocessing::Preprocessing(Eye* eye)
{
    this->eye = eye;
}

Preprocessing::~Preprocessing(){ eye = 0; }

cv::Mat Preprocessing::searchReflection(Eye* eye, int ksize, double c)
{
    cv::Mat mask;
    cv::Mat bgr[3];
    cv::split(*(eye->getEyeImgRes()), bgr);
	cv::adaptiveThreshold(bgr[0], mask, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, ksize, c);
	dilate(mask, mask, cv::Mat(), cv::Point(-1,-1), 2);
    return mask;
}

cv::Mat Preprocessing::inpaintReflection(cv::Mat mask, Eye* eye, int iterations)
{
    cv::Mat imgInp; 
    cv::inpaint(*(eye->getEyeImgRes()), mask, imgInp, iterations, cv::INPAINT_TELEA);
    return imgInp;
}

void Preprocessing::run()
{
    auto[width, height] = obtain_w_h(eye->getEyeImg()->cols, eye->getEyeImg()->rows);
    eye->resize(width, height);
    int ksize = 3; double c = -20;
    cv::Mat mask = searchReflection(eye, ksize, c);
    cv::Mat imgInp = inpaintReflection(mask, eye, 1);
    eye->setImgInp(&imgInp);
    cv::Mat bgr[3];
    cv::split(imgInp, bgr);
    eye->setBlueInp(&bgr[0]);
    eye->setGreenInp(&bgr[1]);
    eye->setRedInp(&bgr[2]);
    for(cv::Mat spec : bgr) spec.release();
    mask.release();
    imgInp.release();
}