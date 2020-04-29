#pragma once
#include <boost/filesystem.hpp>
#include <opencv2/core/mat.hpp>
#include "Eye.hpp"
#include "Preprocessing.hpp"
#include "Segmentation.hpp"
#include <iostream>
#include <boost/timer/timer.hpp>


class IrisSegm
{
private:
    Eye eye;

public:
    IrisSegm(std::string pathToDB);
    ~IrisSegm();
    Eye* getEye();
    void run();
};