#include "LBP.hpp"

// here we're calling the FeatureExtractor (superclass) constructor
LBP::LBP(Subject* sub)
{
    this->sub = sub;
}

LBP::~LBP(void){}

void LBP::encode()
{
    if(this->sub->alreadyExists)
    {
        std::cout << "image already encoded" << std::endl;
        return;
    }
    cv::Mat* img = &(this->sub->normImg);
    for(int i = 1; i < img->rows-1; i++){
        for(int j = 1; j < img->cols-1; j++){
            uchar center = img->at<uchar>(i,j);
            uchar code = 0;
            code |= (img->at<uchar>(i-1,j-1) > center) << 7;
            code |= (img->at<uchar>(i-1,j) > center) << 6;
            code |= (img->at<uchar>(i-1,j+1) > center) << 5;
            code |= (img->at<uchar>(i,j+1) > center) << 4;
            code |= (img->at<uchar>(i+1,j+1) > center) << 3;
            code |= (img->at<uchar>(i+1,j) > center) << 2;
            code |= (img->at<uchar>(i+1,j-1) > center) << 1;
            code |= (img->at<uchar>(i,j-1) > center) << 0;
            this->sub->encodedImg.at<uchar>(i,j) = code;
        }
    }
    img->release();
    img = 0;
}

void LBP::encodeSave()
{
    this->sub->saveTemplate();
}


void LBP::match(Subject* sub1, Subject* sub2){}