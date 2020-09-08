#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <map>


#define NORM_HEIGHT 100
#define NORM_WIDTH 600

using namespace cv;
using namespace std;

/**
 * @file
 * @brief normalizzazione dell'iride tramite Homogeneous Rubber Sheet Model
 */

Mat normalizza(Mat* inp_img, int irisR, int irisX, int irisY, int pupilR, int pupilX, int pupilY, map<string, Point>* mappa);