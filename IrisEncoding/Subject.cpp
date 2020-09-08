#include "Subject.hpp"

Subject::Subject(std::string normPath, std::string maskPath, std::string imgName)
{
    this->normPath = normPath;
    this->imgName = imgName;
    this->normImg = cv::imread(normPath, cv::IMREAD_GRAYSCALE);
    this->mask = cv::imread(maskPath, cv::IMREAD_GRAYSCALE);

    // this->encodedImg = cv::Mat::zeros(100, 600, CV_8UC1);
    // this->alreadyExists = false;
    if(!boost::filesystem::exists("./LBP_Codes/"+imgName+"_LBP.png"))
    {
        this->encodedImg = cv::Mat::zeros(100, 600, CV_8UC1);
        this->alreadyExists = false;
    }
    else
    {
        this->alreadyExists = true;
        this->encodedImg = cv::imread("./LBP_Codes/"+imgName+"_LBP.png",cv::IMREAD_GRAYSCALE);
    }
}

Subject::~Subject()
{
    this->normImg.release();
    this->mask.release();
    this->encodedImg.release();
}

void Subject::saveTemplate()
{
    if(!(this->alreadyExists))
    {
        cv::imwrite("./LBP_Codes/"+imgName+"_LBP.png", this->encodedImg);
        cv::imwrite("./Masks/"+imgName+"_NORMMASK.png", this->mask);
    }
}