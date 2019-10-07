#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

/**
 * @file
 * @brief correzione dei riflessi
 */

void resize_image(Mat* input, Mat* output);
void search_reflection(Mat* input, Mat* mask, int ksize, double c);
void inpaint_reflection(Mat* input, Mat* mask, Mat* output, int iterations);