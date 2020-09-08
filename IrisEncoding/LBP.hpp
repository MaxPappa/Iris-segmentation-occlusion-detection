#pragma once
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "Subject.hpp"
#include <iostream>

class LBP
{
private:
    /* data */
public:
    Subject* sub;

    LBP(Subject* sub);
    ~LBP();

    void encode();
    void match(Subject* sub1, Subject* sub2);
    void encodeSave();
};