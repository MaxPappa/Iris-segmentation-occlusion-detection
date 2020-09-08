#pragma once
#include "Subject.hpp"
#include <opencv2/imgproc.hpp>
#include "hist.hpp"

#define NUM_SLICES 5

enum FeatExtractor{
    LBP_M, BLOB
};

class Matcher
{
private:
    FeatExtractor method;
public:
    Matcher(FeatExtractor method);
    ~Matcher();

    double match_LBP(Subject* sub1, Subject* sub2);
    double match(Subject* sub1, Subject* sub2);
};