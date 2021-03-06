#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include <cmath>

using namespace cv;
using namespace std;

/**
 * @file
 * @brief indici per la valutazione della maschera
 */


float recall(Mat* GTmask, Mat* mask);

float precision(Mat* GTmask, Mat* mask);

float F1_Score(Mat* GTmask, Mat* mask);

float accuracy(Mat* GTmask, Mat* mask);

float specificity(Mat* GTmask, Mat* mask);

double pearson(Mat* maskX, Mat* maskY);

vector<string> getFileNames(char* inp_path);