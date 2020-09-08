#include "preprocessing.h"

uchar freq_neighborhood_value(Mat* src, int x, int y, int ksize){
	map<uchar, size_t> kmap;
	int k = ksize/2;
	if( x - k < 0 || x + k > src->cols || y - k < 0 || y + k > src->rows) return src->at<uchar>(y,x);
	for(int i = x-k; i <= x+k; i++){
		for(int j = y-k; j <= y+k; j++){
			kmap[src->at<uchar>(j,i)]++;
		}
	}
	int max = 0; uchar value = 0;
	for(map<uchar, size_t>::const_iterator ci = kmap.begin(); ci != kmap.end(); ci++){
		if(ci->second > max){
			max = ci->second;
			value = ci->first;
		}
	}
	return value;
}

Mat posterize(Mat* src, int ksize){
	Mat out = Mat::zeros(Size(src->cols, src->rows), CV_8UC1);
	for(int y = 0; y < src->rows; y++){
		for(int x = 0; x < src->cols; x++){
			out.at<uchar>(y,x) = freq_neighborhood_value(src, x, y, ksize);
		}
	}
	return out;
}





