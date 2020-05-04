#include "OcclusionDetector.hpp"

cv::Mat OcclusionDetector::horizKernel = (cv::Mat_<double>(3,3) << 1, 0, -1, 1, 0, -1, 1, 0, -1);   // prewitt
cv::Mat OcclusionDetector::vertKernel = (cv::Mat_<double>(3,3) << 1, 1, 1, 0, 0, 0, -1, -1, -1);    // prewitt
/*	
    cv::Mat OcclusionDetector::horizKernel = (cv::Mat_<double>(3,3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);   // sobel 3x3
	cv::Mat OcclusionDetector::vertKernel = (cv::Mat_<double>(3,3) << -1, -2, -1, 0, 0, 0, 1, 2, 1);    // sobel 3x3
*/

OcclusionDetector::OcclusionDetector(){}

OcclusionDetector::~OcclusionDetector(){}


void OcclusionDetector::drawRays(cv::Mat* upEyelid){
	int x1 = upEyelid->cols/2;
	int y1 = 0;
	double l = upEyelid->cols/4;
	double step = M_PI / (NUM_RAYS-1);
	for(double theta = 0; theta < M_PI; theta+=step){
		int x2 = x1 + cos(theta) * l;
		int y2 = y1 + sin(theta) * l;
		for( double j = l; j >= 0; j-- ){
			int x_p = x1 + cos(theta) * j;
			int y_p = y1 + sin(theta) * j;
			if(y_p >= upEyelid->rows) continue;
			upEyelid->at<uchar>(y_p, x_p) = 160;
		}
	}
	for(double j = l; j >= 0; j--){
		int x_p = x1 + cos(M_PI) * j;
		int y_p = y1 + sin(M_PI) * j;
		upEyelid->at<uchar>(y_p, x_p) = 160;
	}
}

cv::Mat OcclusionDetector::upperEyelidDetection(cv::Mat* normBlue)
{
	cv::Mat upper = cv::Mat::zeros(normBlue->rows, normBlue->cols, CV_8UC1);	// ottengo l'iride normalizzata con al centro l'upper eyelid
	for(int ind = 0; ind < normBlue->cols/2; ind++){
		for(int j = 0; j < normBlue->rows; j++){
			upper.at<uchar>(j,ind) = normBlue->at<uchar>(j,ind+normBlue->cols/2);
		}
	}
	for(int ind = normBlue->cols/2; ind < normBlue->cols; ind++){
		for(int j = 0; j < normBlue->rows; j++){
			upper.at<uchar>(j,ind) = normBlue->at<uchar>(j,ind-normBlue->cols/2);
		}
	}
	cv::Mat upperBlur = upper.clone();
	cv::GaussianBlur( upper, upperBlur, cv::Size(41,41), 0, 0, cv::BORDER_DEFAULT );
	cv::Mat upperEyelidMask = cv::Mat(normBlue->rows, normBlue->cols, CV_8UC1, cv::Scalar(255));
	drawRays(&upper);
	double l_ray = normBlue->cols/4;
	cv::Mat rayPos = (cv::Mat_<cv::Point>(NUM_RAYS, l_ray));
	initRayPos(&upper, &rayPos);
	std::vector<double> vals;
	std::vector<uchar> maxPixels;
	int totPixelsValue = 0;
	cv::Mat coords = cv::Mat_<cv::Point>(NUM_RAYS, 1);
	int index = 0;
	int totVal = 0; int count = 0;
	for(int ray = 0; ray < rayPos.rows; ray++){
		double valMax = 0; int xMax = 0; int yMax = 0;
		for(int x = 10; x < rayPos.cols-10; x++){
			double val;
			int col = rayPos.at<cv::Point>(ray,x).x;
			int row = rayPos.at<cv::Point>(ray,x).y;
			if( ray == 0 && col <= 330+KSIZE/2+1) continue;
			else if(ray == NUM_RAYS-1 && col >= 270-KSIZE/2-1) continue;
			if( upperBlur.at<uchar>(row,col) == 0) continue;
			if(normBlue->at<uchar>(row,col) == 0 || normBlue->at<uchar>(row,col-KSIZE/2) == 0 || normBlue->at<uchar>(row,col+KSIZE/2) == 0) continue;
			if((ray > 0 && ray < NUM_RAYS-1) && (col == 0 || row - KSIZE/2 <= 0)) continue;
			if(rayPos.at<cv::Point>(ray,x).y > upper.rows-10-KSIZE/2) continue;
			if( upperBlur.at<uchar>(row,col) > 150) continue;
			if(ray == 0 || ray == NUM_RAYS-1) val = pixelConvolution(&upper, rayPos.at<cv::Point>(ray,x).x, rayPos.at<cv::Point>(ray,x).y, ray);
			else if(ray > 0 && ray < NUM_RAYS-1) val = pixelConvolution(&upper, rayPos.at<cv::Point>(ray,x).x, rayPos.at<cv::Point>(ray,x).y, ray);
			if(val > valMax){valMax = val; xMax = rayPos.at<cv::Point>(ray,x).x; yMax = rayPos.at<cv::Point>(ray,x).y;}
		}
		coords.at<cv::Point>(0,index++) = cv::Point(xMax,yMax);
		if(ray > 0 && ray < NUM_RAYS-1 && valMax!=0){count++; totVal += valMax;}
		vals.push_back(valMax);
		maxPixels.push_back(upperBlur.at<uchar>(yMax,xMax));
		totPixelsValue += (int)upperBlur.at<uchar>(yMax,xMax);
	}
	int count_pix = 0;
	for(int i = 0; i < maxPixels.size(); i++){
		if(maxPixels[i] >= normBlue->rows)
			count_pix++;
	}
	if(count_pix >= maxPixels.size()*0.75)
		return upperEyelidMask;

	std::vector<double> minimas = localMinima(vals);
	std::vector<cv::Point> vecP;
	std::vector<int> vec_x;
	std::vector<int> vec_y;
	for(int i = 0; i < coords.rows; i++){
		if(find(minimas.begin(), minimas.end(), i) != minimas.end()) continue;
		cv::Point pt = coords.at<cv::Point>(0,i);
		if(pt.y >= upper.rows-1-KSIZE/2) continue;
		if(pt.x == 0) continue;
		vecP.push_back(pt);
		vecP.push_back(cv::Point(pt.x, pt.y*-1));
		vec_x.push_back(pt.x);
		vec_y.push_back(pt.y);
		upperBlur.at<uchar>(pt.y, pt.x) = 255;
	}
	if(!vec_x.empty() && !vec_y.empty()){
		removeOutliers(&vec_x, &vec_y);
	}
	std::vector<double> coeffs = polynomialRegression(vec_x, vec_y, 3);
	int x1 = upper.cols/2;
	int l = (upper.cols/2)-4;
	int x_i = x1-l;
	int x_f = x1+l;
	double y = 0;
	std::vector<cv::Point> polyPoints;
	int countZeros = 0;
	for(int x = x1; x >= x1-l; x--){
		y = 0;
		for(int j = 0; j < coeffs.size(); j++){
			double c = coeffs[j];
			y += c*(pow(x,j));
		}
		if(round(y) >= 0) polyPoints.push_back(cv::Point(x,round(y)));
		if(round(y) == 0) {countZeros++; break;}
	}
	for(int x = x1; x <= x1+l; x++){
		y = 0;
		for(int j = 0; j < coeffs.size(); j++){
			double c = coeffs[j];
			y += c*(pow(x,j));
		}
		if(round(y) >= 0) polyPoints.push_back(cv::Point(x,round(y)));
		if(round(y) == 0) {countZeros++; break;}
	}
	if(countZeros == 2){
		for(int i = 0; i < polyPoints.size(); i++){
			if(polyPoints[i].y >= 0 && polyPoints[i].y < normBlue->rows){
				upperBlur.at<uchar>(polyPoints[i].y, polyPoints[i].x) = 255;
				for(int j = polyPoints[i].y; j >= 0; j--){
					upperEyelidMask.at<uchar>(j, polyPoints[i].x) = 0;
				}
			}
			else if(polyPoints[i].y >= normBlue->rows){
				for(int j = normBlue->rows-1; j >= 0; j--){
					upperEyelidMask.at<uchar>(j, polyPoints[i].x) = 0;
				}
			
			}
		}
	}
	else if(vecP.size() >= 10){
			cv::RotatedRect rotRect = fitEllipse(vecP);
			ellipse(upperBlur, rotRect, cv::Scalar(160));
			ellipse(upperEyelidMask, rotRect, cv::Scalar(0), CV_FILLED);
	}
	cv::Mat reversedUpperMask = cv::Mat::zeros(normBlue->rows, normBlue->cols, CV_8UC1);	// adatto la maschera dell'upper eyelid (è al centro) per quella del lower eyelid (upper è agli estremi e lower al centro)
	for(int ind = 0; ind < upperEyelidMask.cols/2; ind++){
		for(int j = 0; j < upperEyelidMask.rows; j++){
			reversedUpperMask.at<uchar>(j,ind) = upperEyelidMask.at<uchar>(j,ind+upperEyelidMask.cols/2);
		}
	}
	for(int ind = upperEyelidMask.cols/2; ind < upperEyelidMask.cols; ind++){
		for(int j = 0; j < upperEyelidMask.rows; j++){
			reversedUpperMask.at<uchar>(j,ind) = upperEyelidMask.at<uchar>(j,ind-upperEyelidMask.cols/2);
		}
	}
	return reversedUpperMask;
}

void OcclusionDetector::initRayPos(cv::Mat* normBlue, cv::Mat* rayPos){
		int x1 = normBlue->cols/2;
		int y1 = 0;
		double l = normBlue->cols/4;
		double step = M_PI / (NUM_RAYS-1);
		int rays = 0;
		for(double theta = 0; theta < M_PI; theta+=step){
			int h = -1;
			for( double j = l; j >= 0; j-- ){
				h++;
				int xP = x1 + cos(theta) * j;
				int yP = y1 + sin(theta) * j;
				rayPos->at<cv::Point>(rays, h) = cv::Point(xP, yP);
			}
			rays++;
		}
		int h = -1;
		for(double j = l; j >= 0; j--){
			h++;
			int xP = x1 + cos(M_PI) * j;
			int yP = y1 + sin(M_PI) * j;
			rayPos->at<cv::Point>(rays, h) = cv::Point(xP, yP);
		}
}

std::vector<double> OcclusionDetector::localMinima(std::vector<double> vec){
	std::vector<double> minima;
	double v1, v2;
	for(int i = 1; i < vec.size()-1; i++){
		v1 = vec[i-1]; v2 = vec[i+1];
		if( v1 > vec[i] && vec[i] < v2){
			//cout << vec[i] << endl;
			minima.push_back(i);
		}
	}
	return minima;
}

void OcclusionDetector::removeOutliers(std::vector<int>* vec_x, std::vector<int>* vec_y){
	//vector<int> indexes;
	int l = vec_y->size()/2;
	for(int i = 1; i < vec_x->size()-1; i++){
		if(vec_x->at(i-1) > vec_x->at(i) && vec_x->at(i) < vec_x->at(i+1)){
		//	indexes.push_back(vec_x->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
		else if(vec_x->at(i-1) < vec_x->at(i) && vec_x->at(i) > vec_x->at(i+1)){
		//	indexes.push_back(vec_x->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
	}
	for(int i = 1; i < vec_y->size()-1; i++){
		if(vec_y->at(i-1) > vec_y->at(i) && vec_y->at(i) < vec_y->at(i+1) && i > l){
		//	indexes.push_back(vec_y->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
		else if(vec_y->at(i-1) < vec_y->at(i) && vec_y->at(i) > vec_y->at(i+1) && i <= l){
			//indexes.push_back(vec_x->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
	}
	//return indexes;
}

std::vector<double> OcclusionDetector::polynomialRegression(std::vector<int> x, std::vector<int> y, int n){
    int i,j,k,N;
    N = x.size();
    double X[2*n+1];                        //Array that will store the values of sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
    for (i=0;i<2*n+1;i++)
    {
        X[i]=0;
        for (j=0;j<N;j++)
            X[i]=X[i]+pow(x[j],i);        //consecutive positions of the array will store N,sigma(xi),sigma(xi^2),sigma(xi^3)....sigma(xi^2n)
    }
    double B[n+1][n+2],a[n+1];            //B is the Normal matrix(augmented) that will store the equations, 'a' is for value of the final coefficients
    for (i=0;i<=n;i++)
        for (j=0;j<=n;j++)
            B[i][j]=X[i+j];            //Build the Normal matrix by storing the corresponding coefficients at the right positions except the last column of the matrix
    double Y[n+1];                    //Array to store the values of sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
    for (i=0;i<n+1;i++)
    {    
        Y[i]=0;
        for (j=0;j<N;j++)
        Y[i]=Y[i]+pow(x[j],i)*y[j];        //consecutive positions will store sigma(yi),sigma(xi*yi),sigma(xi^2*yi)...sigma(xi^n*yi)
    }
    for (i=0;i<=n;i++)
        B[i][n+1]=Y[i];                //load the values of Y as the last column of B(Normal Matrix but augmented)
    n=n+1;                //n is made n+1 because the Gaussian Elimination part below was for n equations, but here n is the degree of polynomial and for n degree we get n+1 equations
    for (i=0;i<n;i++)                    //From now Gaussian Elimination starts(can be ignored) to solve the set of linear equations (Pivotisation)
        for (k=i+1;k<n;k++)
            if (B[i][i]<B[k][i])
                for (j=0;j<=n;j++)
                {
                    double temp=B[i][j];
                    B[i][j]=B[k][j];
                    B[k][j]=temp;
                }
    for (i=0;i<n-1;i++)            //loop to perform the gauss elimination
        for (k=i+1;k<n;k++)
            {
                double t=B[k][i]/B[i][i];
                for (j=0;j<=n;j++)
                    B[k][j]=B[k][j]-t*B[i][j];    //make the elements below the pivot elements equal to zero or elimnate the variables
            }
    for (i=n-1;i>=0;i--)                //back-substitution
    {                        //x is an array whose values correspond to the values of x,y,z..
        a[i]=B[i][n];                //make the variable to be calculated equal to the rhs of the last equation
        for (j=0;j<n;j++)
            if (j!=i)            //then subtract all the lhs values except the coefficient of the variable whose value                                   is being calculated
                a[i]=a[i]-B[i][j]*a[j];
        a[i]=a[i]/B[i][i];            //now finally divide the rhs by the coefficient of the variable to be calculated
    }
    std::vector<double> coeffs;
    for(i=0;i<n;i++)
        coeffs.push_back(a[i]);
    return coeffs;
}

double OcclusionDetector::pixelConvolution(cv::Mat* normBlue, int x, int y, int ray){
	double horizontalVal = 0;
	double verticalVal = 0;
	if(ray == 0 || ray == NUM_RAYS-1){
		for(int i = -1*KSIZE/2; i <= KSIZE/2; i++){
			for(int j = 0; j <= KSIZE/2; j++){
				horizontalVal += ((int)(normBlue->at<uchar>(y + j, x + i))) * horizKernel.at<double>(j+KSIZE/2,i+KSIZE/2);
				verticalVal += ((int)(normBlue->at<uchar>(y + j, x + i))) * vertKernel.at<double>(j+KSIZE/2,i+KSIZE/2);
			}
		}
		//return abs(horizontalVal) + abs(verticalVal); // posso ritornare sia questo che quello sotto, la differenza è minima (anche se più preciso quello sotto)
		return sqrt(pow(horizontalVal, 2) + pow(verticalVal, 2));
	}
	else{
		for(int i = -1*KSIZE/2; i <= KSIZE/2; i++){
			for(int j = -1*KSIZE/2; j <= KSIZE/2; j++){
				if((y+j) >= 0){
					horizontalVal += normBlue->at<uchar>(y + j, x + i) * horizKernel.at<double>(j+KSIZE/2,i+KSIZE/2);
					verticalVal += normBlue->at<uchar>(y + j, x + i) * vertKernel.at<double>(j+KSIZE/2,i+KSIZE/2);
				}
			}
		}
		//return abs(horizontalVal) + abs(verticalVal);
		return sqrt(pow(horizontalVal, 2) + pow(verticalVal, 2));
	}
}


cv::Mat OcclusionDetector::threshReflectionDetection(cv::Mat* normBlue)
{
	cv::Mat reflectionMask;
	//GaussianBlur(*normImg, reflectionMask, Size(3,3), 0, 0);
	cv::Mat bgr[3];
	cv::split(*normBlue , bgr);
	cv::threshold(bgr[0], reflectionMask, 200, 255, cv::THRESH_BINARY);
	//adaptiveThreshold(reflectionMask, reflectionMask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, ksize, c);
	return reflectionMask;
}

cv::Mat OcclusionDetector::lowerEyelidDetection(cv::Mat* normRed){
	cv::Mat lowerEyelidMask = cv::Mat(normRed->rows, normRed->cols, CV_8UC1, cv::Scalar(255));
	cv::Mat meanMat, stdDevMat;
	cv::Mat mask = cv::Mat::zeros(normRed->rows, normRed->cols, CV_8UC1);
	for(int y = 0; y <= normRed->rows/2; y++){
		for(int x = normRed->cols/4; x <= (3*normRed->cols)/4; x++){
			mask.at<int>(y,x) = 1;
		}
	}
	cv::meanStdDev(*normRed, meanMat, stdDevMat, mask);
	double stdDev = stdDevMat.at<double>(0,0);
	double mean = meanMat.at<double>(0,0);
	int threshold = (int)(mean + stdDev);
	if(stdDev > mean/4){
		//uchar threshold = (uchar)(mean + stdDev/2);
		for(int y = 0; y < normRed->rows/2; y++){
			for(int x = 0; x < normRed->cols; x++){
				if(((int)normRed->at<uchar>(y,x)) > threshold)
					lowerEyelidMask.at<uchar>(y,x) = 0;
			}
		}
	}
	return lowerEyelidMask;
}


void OcclusionDetector::run(Eye* eye)
{	
	cv::Mat bgr[3];
	cv::split(*(eye->getNormImg()), bgr);
	std::cout << 1 << std::endl;
	cv::Mat reflectionMask = threshReflectionDetection(&bgr[0]);
	std::cout << 2 << std::endl;
	cv::Mat lowEyelidMask = lowerEyelidDetection(&bgr[2]);
	std::cout << 3 << std::endl;
	cv::Mat upEyelidMask = upperEyelidDetection(&bgr[2]);
	std::cout << 4 << std::endl;
	cv::Mat normMask = cv::Mat(lowEyelidMask.rows, lowEyelidMask.cols, CV_8UC1, cv::Scalar(255));
	std::cout << 5 << std::endl;
	cv::Mat* binMask = eye->getBinMask();
	std::cout << 6 << std::endl;
	std::map<size_t, cv::Point> coords = *(eye->getEyeCoords());
	size_t seed = 0;
	std::map<size_t, int> kv;
	for(int y = 0; y < normMask.rows; y++){
		for(int x = 0; x < normMask.cols; x++){
			if(lowEyelidMask.at<uchar>(y,x) == 0 || upEyelidMask.at<uchar>(y,x) == 0 || reflectionMask.at<uchar>(y,x) != 0){
				normMask.at<uchar>(y,x) = 0;
				boost::hash_combine(seed, y);
                boost::hash_combine(seed, x);
				cv::Point pt = coords[seed];
				binMask->at<uchar>(pt.y, pt.x) = 0;
				seed = 0;
			}
		}
	}
	cv::resize(normMask, normMask, cv::Size(NORM_WIDTH, NORM_HEIGHT));
	eye->setNormMask(&normMask);
	cv::Mat normImg = *(eye->getNormImg());
	cv::resize(normImg, normImg, cv::Size(NORM_WIDTH, NORM_HEIGHT));
	eye->setNormImg(&normImg);
	// here binMask is modifed in the eye* ptr, so there's no need to re-set the binMask in the eye object.
	/*cv::imshow("ReflectionMask Show", reflectionMask);
	cv::imshow("LowEyelidMask Show", lowEyelidMask);
	cv::imshow("UpEyelidMask Show", upEyelidMask);
	cv::imshow("BinMask Show", *binMask);
	cv::imshow("NormMask Show", normMask);
	cv::waitKey(0);
	cv::destroyAllWindows();*/
	std::cout << 7 << std::endl;
	reflectionMask.release();
	lowEyelidMask.release();
	upEyelidMask.release();
	normMask.release();
	normImg.release();
}