#include "Eye.hpp"

Eye::Eye(std::string eyePath)
{
    this->eyePath = eyePath;
    this->eyeImg = cv::imread(eyePath,1);
}

Eye::~Eye()
{
    this->eyeImg.release();
    this->eyeImgRes.release();
    this->mask.release();
    this->imgInp.release();
    this->redSpec.release();
    this->greenSpec.release();
    this->blueSpec.release();
    std::cout << "every field of Eye object has been released" << std::endl;
}

// getter
std::string Eye::getEyePath(){ return this->eyePath; }

int Eye::getImgHeight(){ return this->resHeight; }
int Eye::getImgWidth(){ return this->resWidth; }
int Eye::getIrisRadius(){ return this->irisRadius; }
double Eye::getIrisValue(){ return this->irisValue; }
cv::Point Eye::getIrisCenter(){ return this->irisCenter; }
int Eye::getPupilRadius(){ return this->pupilRadius; }
double Eye::getPupilValue(){ return this->pupilValue; }
cv::Point Eye::getPupilCenter(){ return this->pupilCenter; }


cv::Mat* Eye::getEyeImg(){ return &(this->eyeImg); }
cv::Mat* Eye::getEyeImgRes(){ return &(this->eyeImgRes); }
cv::Mat* Eye::getMask(){ return &(this->mask); }
cv::Mat* Eye::getImgInp(){ return &(this->imgInp); }
cv::Mat* Eye::getBlueSpec(){ return &(this->blueSpec); }
cv::Mat* Eye::getGreenSpec(){ return &(this->greenSpec); }
cv::Mat* Eye::getRedSpec(){ return &(this->redSpec); }
cv::Mat* Eye::getBlueInp(){ return &(this->blueInp); }
cv::Mat* Eye::getGreenInp(){ return &(this->greenInp); }
cv::Mat* Eye::getRedInp(){ return &(this->redInp); }

// setter
void Eye::setMask(cv::Mat* mask){ this->mask = *mask;}
void Eye::setImgInp(cv::Mat* imgInp){ this->imgInp = *imgInp; }
void Eye::setBlueSpec(cv::Mat* spec){ this->blueSpec = *spec; }
void Eye::setGreenSpec(cv::Mat* spec){ this->greenSpec = *spec; }
void Eye::setRedSpec(cv::Mat* spec){ this->redSpec = *spec; }
void Eye::setBlueInp(cv::Mat* specInp){ this->blueInp = *specInp; }
void Eye::setGreenInp(cv::Mat* specInp){ this->greenInp = *specInp; }
void Eye::setRedInp(cv::Mat* specInp){ this->redInp = *specInp; }
void Eye::setIrisCenter(cv::Point center){ this->irisCenter = center; }
void Eye::setIrisRadius(int radius){ this->irisRadius = radius; }
void Eye::setIrisValue(double value){ this->irisValue = value; }
void Eye::setPupilCenter(cv::Point center){ this->pupilCenter = center; }
void Eye::setPupilRadius(int radius){ this->pupilRadius = radius; }
void Eye::setPupilValue(double value){ this->pupilValue = value; }

void Eye::resize(int width, int height)
{
    std::cout << "width " << width << " , height " << height << std::endl;
    cv::resize((this->eyeImg), this->eyeImgRes, cv::Size(width, height));
    this->resWidth = width; this->resHeight = height;
    cv::Mat bgr[3]; //= {this->blueSpec,this->greenSpec,this->redSpec};
    cv::split(this->eyeImgRes, bgr);
    this->blueSpec = bgr[0];
    this->greenSpec = bgr[1];
    this->redSpec = bgr[2];
}