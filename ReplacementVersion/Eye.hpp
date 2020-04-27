#pragma once
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include <iostream>

class Eye
{
    private:
        std::string eyePath; // absolute path
        int resWidth, resHeight; // width and height of the eyeImgRes (so dimensions post-resizing)
        int irisRadius; cv::Point irisCenter; double irisValue;
        int pupilRadius; cv::Point pupilCenter; double pupilValue;
        cv::Mat eyeImg;
        cv::Mat eyeImgRes;  // same image as eyeImg but resized with the resize method
        cv::Mat mask;
        cv::Mat imgInp;
        cv::Mat blueSpec, greenSpec, redSpec;

    public:
        Eye(std::string eyePath);
        ~Eye();
        
        void resize(int width, int height);

        // getter
        std::string getEyePath();
        cv::Mat* getEyeImg();
        int getImgHeight();
        int getImgWidth();
        cv::Mat* getEyeImgRes();
        cv::Mat* getMask();
        cv::Mat* getImgInp();
        cv::Mat* getBlueSpec();
        cv::Mat* getGreenSpec();
        cv::Mat* getRedSpec();
        int getIrisRadius();
        double getIrisValue();
        cv::Point getIrisCenter();
        int getPupilRadius();
        double getPupilValue();
        cv::Point getPupilCenter();

        // setter
        void setMask(cv::Mat* mask);
        void setBlueSpec(cv::Mat* spec);
        void setGreenSpec(cv::Mat* spec);
        void setRedSpec(cv::Mat* spec);
        void setIrisRadius(int radius);
        void setIrisCenter(cv::Point center);
        void setIrisValue(double value);
        void setPupilRadius(int radius);
        void setPupilCenter(cv::Point center);
        void setPupilValue(double value);
};