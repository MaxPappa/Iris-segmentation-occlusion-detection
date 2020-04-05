#include "eye_localization.h"

/**
 * @param ki vale 4.8 oppure 15.8, è il valore corrispondente a ki = c/hi
 * @param c_length è la lunghezza della corda
 * @return c*( ((ki)^2 +4) / (8*ki) )
 */
float obtain_radii(float ki, float c_length){
	float num = pow(ki, 2) + 4;
	float tmp = 8 * ki;
	float result = c_length * (num / tmp);
	return result;
}

/**
 * @param input Matrice di input contenente l'immagine su cui applicare Canny
 * @param output Matrice di output in cui andrà il risultato di Canny
 * @param sigma valore posto di default a 0.33
 */
void auto_canny(Mat* input, Mat* output, float sigma){
	Mat bil;
	bilateralFilter(*input, bil, 9, 100, 100);
	float val = mean(bil)[0]; // mean(input) ritorna uno Scalar.
	int lower_t = max(0, (int)((1.0 - sigma) * val));
	int upper_t = min(255, (int)((1.0 + sigma) * val));
	
	Canny(bil, *output, lower_t, upper_t, 3, true);
}

bool isVertical(string imgPath)
{
	Mat img = imread(imgPath, CV_LOAD_IMAGE_COLOR);
	float height = img.rows;
	float width = img.cols;
	if ((height/16 == width/9) || (height/4 == width/3))
			return true;
	return false;
}

Mat threshSkinImg(Mat img)
{
	Mat mat(img);
	int coeff = (int)(img.cols/256);
	if (coeff > 1)
		resize(mat, mat, Size((int)(img.cols/coeff), (int)(img.rows/coeff)));
	Mat blankImg(mat.size(), CV_8U, Scalar(255));

	Mat bgra, hsv, ycrcb;
	
	cvtColor(mat, bgra, COLOR_BGR2BGRA);
	cvtColor(mat, hsv, COLOR_BGR2HSV);
	cvtColor(mat, ycrcb, COLOR_BGR2YCrCb);

	Mat ycrcb_c[3], bgra_c[4], hsv_c[3];
	split(ycrcb, ycrcb_c);
	split(bgra, bgra_c);

	split(hsv, hsv_c);
	Mat sSpec = hsv_c[1];
	Mat sSpecDouble;
	sSpec.convertTo(sSpecDouble, CV_32F, 0, 1);
	Scalar intensity;
	for (int x = 0; x < sSpec.cols; ++x)
	{
		for (int y = 0; y < sSpec.rows; ++y)
		{
			intensity = sSpec.at<uchar>(y, x);
			sSpecDouble.at<float>(y, x) = intensity.val[0]/255.0;
		}
	}

	vector<std::pair<int, int>> lstSkin;
	float b, g, r, a;
	float y, cr, cb;
	float h, s, v;
	Scalar value;
	for (int j = 0; j < mat.rows; ++j)
	{
		for (int i = 0; i < mat.cols; ++i)
		{
			value = bgra_c[0].at<uchar>(j, i);
			b = value.val[0];
			value = bgra_c[1].at<uchar>(j, i);
			g = value.val[0];
			value = bgra_c[2].at<uchar>(j, i);
			r = value.val[0];
			value = bgra_c[3].at<uchar>(j, i);
			a = value.val[0];

			value = ycrcb_c[0].at<uchar>(j, i);
			y = value.val[0];
			value = ycrcb_c[1].at<uchar>(j, i);
			cr = value.val[0];
			value = ycrcb_c[2].at<uchar>(j, i);
			cb = value.val[0];

			value = hsv_c[0].at<uchar>(j, i);
			h = value.val[0];
			value = sSpecDouble.at<float>(j, i);
			s = value.val[0];
			
			value = hsv_c[2].at<uchar>(j, i);
			v = value.val[0];

			if (h>=0 && h<=50 && s>=0.23 && s<=0.68 && r>95 && g>40 && b>20 && r>g && r>b && abs(r-g)>15 && a>15)
				lstSkin.push_back(std::pair<int, int>(i, j));
			else if (r>95 && g>40 && b>20 && r>g && r>b && abs(r-g)>15 && a>15 && cr>135 && cb>85 && y>80 && cr<=((1.5862*cb)+20) && cr>=((0.3488*cb)+76.2069) && cr>=((-4.5652*cb)+234.5652) && cr<=((-1.15*cb)+301.75) && cr<=((-2.2857*cb)+432.85))
				lstSkin.push_back(std::pair<int, int>(i, j));
		}
	}

	for (std::pair<int, int> p : lstSkin)
		blankImg.at<uchar>(std::get<1>(p), std::get<0>(p)) = 0.0;
	//cout << lstSkin.size() << endl;
	return blankImg;
}

float* clip(float min, float max, float* values, int v_size)
{
	float* clipped = (float*)calloc(v_size, sizeof(float));
	for (int i = 0; i < v_size; ++i)
	{
		if (values[i] < min)
			*(clipped + i) = min;
		else if (values[i] > max)
			*(clipped + i) = max;
		else
			*(clipped + i) = values[i];
	}
	return clipped;
}

bool shouldUseVJ(string imgPath)
{
	ofstream log;
	log.open("./log.txt", std::ios_base::app);
	if (isVertical(imgPath))
	{
		log << "For " + imgPath + "you should use VJ because image is vertical" << endl;
		return true;
	}

	Mat img = imread(imgPath, 1);
	int coeff = (int)(img.rows/256);
	if (coeff > 1)
		resize(img, img, Size((int)(img.rows/coeff), (int)(img.cols/coeff)));
	int alpha = 1, beta = 30;
	Mat newImage = Mat::zeros(img.size(), img.type());
	float bSpec, gSpec, rSpec;
	float mean_val = (mean(img).val[0] + mean(img).val[1] + mean(img).val[2]) / 3;
	Mat bgr[3];
	split(img, bgr);
	Scalar value;
	float f_value;
	/*if (mean_val > 150)
	{
		for (int y = 0; y < img.rows; ++y)
		{
			for (int x = 0; x < img.cols; ++x)
			{
				value = bgr[0].at<uchar>(y, x);
				f_value = alpha*value[0]-beta;
				bSpec = f_value < 0 ? 0 : f_value;

				value = bgr[1].at<uchar>(y, x);
				f_value = alpha*value[0]-beta;
				gSpec = f_value < 0 ? 0 : f_value;

				value = bgr[2].at<uchar>(y, x);
				f_value = alpha*value[0]-beta;
				rSpec = f_value < 0 ? 0 : f_value;
				Scalar s(bSpec, gSpec, rSpec);
				newImage.at<Scalar>(y, x) = s;
			}
		}
	}
	else if (mean_val < 80)
	{
		for (int y = 0; y < img.rows; ++y)
		{
			for (int x = 0; x < img.cols; ++x)
			{
				value = bgr[0].at<uchar>(y, x);
				f_value = alpha*value[0]+beta;
				bSpec = f_value > 255 ? 255 : f_value;

				value = bgr[1].at<uchar>(y, x);
				f_value = alpha*value[0]+beta;
				gSpec = f_value > 255 ? 255 : f_value;

				value = bgr[2].at<uchar>(y, x);
				f_value = alpha*value[0]+beta;
				rSpec = f_value > 255 ? 255 : f_value;

				Scalar s(bSpec, gSpec, rSpec);
				//cout << s.val[0] <<"\t" << s.val[1] << "\t" << s.val[2] << endl;
				newImage.at<Scalar>(y, x) = s;
			}
			cout << "y=" << y << ", rows=" << img.rows << endl;
		}
		cout << "finito" << endl;
	}*/
	Mat blankImg = threshSkinImg(newImage);
	log << "finito BlankImg" << endl;
	log << "blankImg rows =" << blankImg.rows << ", cols=" << blankImg.cols << endl;
	/*namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
	imshow("Display window", blankImg);
	waitKey(0);
	destroyAllWindows();*/
	dilate(blankImg, blankImg, Mat());
	int cB = 0, cW = 0;
	for (int y = 0; y < blankImg.rows; ++y)
	{
		for (int x = 0; x < blankImg.cols; ++x)
		{
			value = blankImg.at<uchar>(y, x);
			if (value.val[0] == 0)
				cB++;
			else
				cW++;
		}
	}

	if (((float)cW) / (newImage.rows*newImage.cols) < 0.3)
	{
		log << "cW = " << cW << " % = " << ((double)cW / (newImage.rows*newImage.cols)) << endl;
		log << "For " << imgPath.c_str() << " you should use VJ\n";
		return true;
	}
	else
	{
		log << "cW = " << cW << ", % = " << ((double)cW / (newImage.rows*newImage.cols)) << endl;
		log << "For " << imgPath.c_str() << " you should NOT use VJ\n" << endl;
		return false;
	}
} 