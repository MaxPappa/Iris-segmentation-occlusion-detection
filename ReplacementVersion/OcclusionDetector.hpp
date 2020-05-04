#pragma once
#include "Eye.hpp"
#include <boost/functional/hash/hash.hpp>
#include <iostream>

#define NUM_RAYS 46
#define KSIZE 3
#define NORM_HEIGHT 100
#define NORM_WIDTH 600

class OcclusionDetector
{
private:
    static cv::Mat horizKernel;
    static cv::Mat vertKernel;

    void drawRays(cv::Mat* normRed);
    void initRayPos(cv::Mat* normRed, cv::Mat* rayPos);
    double pixelConvolution(cv::Mat* normRed, int x, int y, int ray);
    cv::Mat upperEyelidDetection(cv::Mat* normRed);
    std::vector<double> localMinima(std::vector<double> vec);
    void removeOutliers(std::vector<int>* vec_x, std::vector<int>* vec_y);
    cv::Mat lowerEyelidDetection(cv::Mat* normRed);
    cv::Mat threshReflectionDetection(cv::Mat* normBluev);
    std::vector<double> polynomialRegression(std::vector<int> x, std::vector<int> y, int n);

    
public:
    OcclusionDetector();
    ~OcclusionDetector();
    void run(Eye* eye);
};