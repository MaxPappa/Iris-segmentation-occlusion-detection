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
    void searchReflection(Eye* eye, int ksize, double c);
    void inpaintReflection(Eye* eye, int iterations);

public:
    Preprocessing(Eye* eye);
    ~Preprocessing();
    void run();
};