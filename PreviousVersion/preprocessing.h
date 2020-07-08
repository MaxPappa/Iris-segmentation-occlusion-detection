#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <map>

using namespace cv;
using namespace std;

/**
 * @file
 * @brief preprocessing da effettuare (e continuare ad implementare) prima della segmentazione dell'iride
 */

uchar freq_neighborhood_value(Mat* src, int x, int y, int ksize);

Mat posterize(Mat* src, int ksize);