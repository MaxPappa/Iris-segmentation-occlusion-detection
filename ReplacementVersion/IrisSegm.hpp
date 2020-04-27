#pragma once
#include <boost/filesystem.hpp>
#include <opencv2/core/mat.hpp>
#include "Eye.hpp"
#include "utils.hpp"
#include "Preprocessing.hpp"
#include "Segmentation.hpp"
#include <iostream>

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