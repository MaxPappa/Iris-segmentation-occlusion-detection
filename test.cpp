#include "test.h"


double pearson(Mat* maskX, Mat* maskY){
	// convert data-type to "float"
	Mat im_float_1;
	maskX->convertTo(im_float_1, CV_32F);
	Mat im_float_2;
	maskY->convertTo(im_float_2, CV_32F);

	int n_pixels = im_float_1.rows * im_float_1.cols;

	// Compute mean and standard deviation of both images
	Scalar im1_Mean, im1_Std, im2_Mean, im2_Std;	
	meanStdDev(im_float_1, im1_Mean, im1_Std);
	meanStdDev(im_float_2, im2_Mean, im2_Std);

	// Compute covariance and correlation coefficient
	double covar = (im_float_1 - im1_Mean).dot(im_float_2 - im2_Mean) / n_pixels;
	double correl = covar / (im1_Std[0] * im2_Std[0]);
	cout << correl << endl;
	return correl;
}

 
/**
 * @brief calcolo del Recall
 * @param GTmask maschera Ground Truth
 * @param mask maschera sul quale valutare il Recall
 * @return valore del Recall: |TP| / (|FN|+|TP|)
 */
float recall(Mat* GTmask, Mat* mask){
	float num = 0;
	float denom = 0;
	for(int y = 0; y < GTmask->rows; y++){
		for(int x = 0; x < GTmask->cols; x++){
			if(GTmask->at<uchar>(y,x) > 0){
				denom++;
				if(mask->at<uchar>(y,x) > 0)
					num++;
			}
		}
	}
	return num/denom;
}

/**
 * @brief calcolo della Precision
 * @param GTmask maschera Ground Truth
 * @param mask maschera sul quale valutare la Precision
 * @return valore della Precision: |TP| / (|TP|+|FP|)
 */
float precision(Mat* GTmask, Mat* mask){
	float num = 0;
	float denom = 0;
	for(int y = 0; y < mask->rows; y++){
		for(int x = 0; x < mask->cols; x++){
			if(mask->at<uchar>(y,x) > 0){
				denom++;
				if(GTmask->at<uchar>(y,x) > 0)
					num++;
			}
		}
	}
	return num/denom;
}

/**
 * @brief calcolo dell'F1-Score
 * @param GTmask maschera Ground Truth
 * @param mask maschera sul quale valutare l'F1-Score
 * @return valore dell'F1-Score: 2*((Precision * Recall) / (Precision + Recall))
 */
float F1_Score(Mat* GTmask, Mat* mask){
	float rec = recall(GTmask, mask);
	float prec = precision(GTmask, mask);
	return 2 * ((prec*rec) / (prec+rec));
}


/**
 * @brief calcolo dell'accuracy (date due maschere, delle quali una ground truth).
 * @param GTmask maschera Ground Truth
 * @param mask maschera sul quale valutare l'accuracy
 * @return valore dell'accuracy: (|TN|+|TP|) / (|Number Of Pixels|)
 */
float accuracy(Mat* GTmask, Mat* mask){
	float num = 0;
	float denom = 0;
	for(int y = 0; y < mask->rows; y++){
		for(int x = 0; x < mask->cols; x++){
			if((GTmask->at<uchar>(y,x) == 0 && mask->at<uchar>(y,x) == 0) || (GTmask->at<uchar>(y,x) > 0 && mask->at<uchar>(y,x) > 0))
				num++;
			denom++;
		}
	}
	return num/denom;
}


/**
 * @brief calcolo della specificity (date due maschere, delle quali una ground truth).
 * @param GTmask maschera Ground Truth
 * @param mask maschera sul quale valutare la specificity
 * @return valore della specificity: |TN|/(|TN+FP|)
 */
float specificity(Mat* GTmask, Mat* mask){
	float falsePos = 0;
	float trueNeg = 0;
	for(int y = 0; y < mask->rows; y++){
		for(int x = 0; x < mask->cols; x++){
			if(GTmask->at<uchar>(y,x) == 0){
				if(mask->at<uchar>(y,x) > 0) falsePos++;
				trueNeg++;
			}
		}
	}
	return trueNeg/(trueNeg+falsePos);
}



vector<string> getFileNames(char* inp_path){
	vector<string> out_vec;
	DIR *inp_dir;
	struct dirent *ent;
	string extension;
	if ((inp_dir = opendir(inp_path)) != NULL) {
		while ((ent = readdir (inp_dir)) != NULL) {
			if(ent->d_name[0] == '.') continue;
			extension = ent->d_name;
			if(ent->d_name[strlen(ent->d_name)-1] == 'G' || ent->d_name[strlen(ent->d_name)-1] == 'g' || ent->d_name[strlen(ent->d_name)-1] == 'f'){		// controllo solo se è JPG ma per ora mi basta solo la G
				out_vec.push_back(ent->d_name);
			}
		}
		closedir(inp_dir);
	}
	return out_vec;
}