#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <numeric>
#include <sys/stat.h>
#include <string.h>

#define DELTA_R 7
#define DELTA_PUP 5
#define SIGMA -1

using namespace cv;
using namespace std;

/**
 * @file
 * @brief individuazione dell'iride e della pupilla
 */

struct image {
	Mat* blue;
	Mat* red;
	Mat* green;
	Mat* grey;
	int rows;
	int cols;
};

struct results {
	int radius;
	Point center;
	double value;
};

uchar pixel_value(image* img, double angle, Point centro, int r);

uchar pixel_value_multi(image* img, double angle, Point centro, int r, string col);

int contour_sum(image* img, Point centro, int r);

int contour_sum_multi(image* img, Point centro, int r, string col);

vector<int> linear_integral_vector(image* img, Point centro, vector<int> radius_range);

vector<int> linear_integral_vector_multi(image* img, Point centro, vector<int> radius_range, string col);

pair<int,int> obtain_w_h(int cols, int rows);

pair<int,int> obtain_w_h_miche(int cols, int rows);

int checkWidth(int width);

results* apply_daugman_operator(image* img, int r_min, int r_max);

results* apply_daugman_operator_multi(image* img, int r_min, int r_max);

results* pupil_daugman_operator(Mat* img_red, int r_min, int r_max);

vector<int> pupil_linear_integral_vector(Mat* img_red, Point centro, vector<int> radius_range);

int pupil_contour_sum(Mat* img_red, Point centro, int r);

uchar pupil_pixel_value(Mat* img_red, double angle, Point centro, int r);

int pup_contour_divider(Mat* img_red, vector<int> radius_range, int pos, Point centro);

Mat pup_convolution(Mat* img_red, double sigma, vector<int>* line_int, vector<int> radius_range, Point centro);

Mat convolution(double sigma, vector<int>* line_int);

double pixel_conv(vector<int>* line_int, vector<double>* kernel, int pos);

Mat binaryMask(Mat* src_image, int iris_r, Point iris_c, int pupil_r, Point pupil_c);

vector<string> split(string strToSplit, char delimeter);