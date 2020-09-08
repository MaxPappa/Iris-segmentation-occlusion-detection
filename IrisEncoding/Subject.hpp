#pragma once
#include <string>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <boost/filesystem.hpp>

class Subject
{
private:

public:
    std::string normPath; // normalized iris path
    std::string imgName;
    bool alreadyExists;
    cv::Mat normImg;    // normalized iris image
    cv::Mat mask;       // mask of normalized iris image (normImg)
    cv::Mat encodedImg; // normImg encoded by using some FeatureExtractor (e.g.: LBP or BLOB)

    void saveTemplate();

    Subject(std::string normPath, std::string maskPath, std::string imgName);
    ~Subject();
};