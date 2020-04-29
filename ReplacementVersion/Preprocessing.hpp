#pragma once
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include "utils.hpp"
#include "Eye.hpp"

class Preprocessing
{
private:
    Eye* eye;
    cv::Mat searchReflection(Eye* eye, int ksize, double c);
    cv::Mat inpaintReflection(cv::Mat mask, Eye* eye, int iterations);

public:
    Preprocessing(Eye* eye);
    ~Preprocessing();
    void run();
};