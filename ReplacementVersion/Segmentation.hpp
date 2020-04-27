#include "Eye.hpp"
#include <numeric>
#include "utils.hpp"

#define DELTA_R 7
#define DELTA_PUP 5
#define SIGMA -1

enum eEyePart{iris, pupil};

class Segmentation
{
private:
    int rMin; // min radius to take account for limbus search
	int rMax; // max radius to take account for limbus search
    Eye* eye;

    uchar pixelValue(Eye* eye, double angle, cv::Point center, int r);
    int contourSum(Eye* eye, cv::Point centro, int r);
    std::vector<int> linearIntegVec(Eye* eye, cv::Point center, std::vector<int> radiusRange);
    void daugmanOperator(eEyePart);
    cv::Mat convolution(Eye* eye, vector<int>* lineInt, eEyePart eyePart, cv::Point center, std::vector<int> radiusRange);    double pixelConv(vector<int>* lineInt, std::vector<double>* kernel, int pos);
    int pupContourDivider(Eye* eye, std::vector<int> radiusRange, int pos, cv::Point center);

public:
    Segmentation(Eye* eye);
    ~Segmentation();
    void run();

    // getter
    int getRMin();
    int getRMax();
    Eye* getEye();

};
