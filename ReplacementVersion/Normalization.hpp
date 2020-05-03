#pragma once
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <map>
#include "Eye.hpp"
#include <boost/functional/hash/hash.hpp>
#include "utility"

#define NORM_HEIGHT 100
#define NORM_WIDTH 600

class Normalization
{
private:
    Eye* eye;
    void normalize(Eye* eye, std::map<size_t, cv::Point>* coords);
    cv::Mat binaryMask(Eye* eye);

public:
    Normalization();
    ~Normalization();
    void run(Eye* eye);
};