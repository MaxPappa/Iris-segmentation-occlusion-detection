#include "occlusion_reflection_detec.h"

Mat sobel;
Mat horizontalK;	// kernel per i cambiamenti orizzontali
Mat verticalK;		// kernel per i cambiamenti verticali
Mat rayPos;
Mat mask;


/**
 * @brief sull'iride normalizzata di input vengono disegnati NUM_RAYS (46, in base all'header) partendo dalla posizione
 *		centrale superiore, ovvero partendo da (x,y) = (width/2,0).
 * @param norm_img immagine normalizzata a singolo spettro (spettro rosso)
 */
int drawRays(Mat* norm_img){
	int x1 = norm_img->cols/2;
	int y1 = 0;
	double l = norm_img->cols/4;
	double step = M_PI / (NUM_RAYS-1);
	for(double theta = 0; theta < M_PI; theta+=step){
		int x2 = x1 + cos(theta) * l;
		int y2 = y1 + sin(theta) * l;
		for( double j = l; j >= 0; j-- ){
			int x_p = x1 + cos(theta) * j;
			int y_p = y1 + sin(theta) * j;
			if(y_p >= norm_img->rows) continue;
			norm_img->at<uchar>(y_p, x_p) = 160;
		}
	}
	for(double j = l; j >= 0; j--){
		int x_p = x1 + cos(M_PI) * j;
		int y_p = y1 + sin(M_PI) * j;
		norm_img->at<uchar>(y_p, x_p) = 160;
	}
	return 0;
}


int initRayPos(Mat* normImg){
		int x1 = normImg->cols/2;
		int y1 = 0;
		double l = normImg->cols/4;
		rayPos = (Mat_<Point>(NUM_RAYS, l));
		double step = M_PI / (NUM_RAYS-1);
		int rays = 0;
		for(double theta = 0; theta < M_PI; theta+=step){
			int h = -1;
			for( double j = l; j >= 0; j-- ){
				h++;
				int x_p = x1 + cos(theta) * j;
				int y_p = y1 + sin(theta) * j;
				rayPos.at<Point>(rays, h) = Point(x_p, y_p);
			}
			rays++;
		}
		int h = -1;
		for(double j = l; j >= 0; j--){
			h++;
			int x_p = x1 + cos(M_PI) * j;
			int y_p = y1 + sin(M_PI) * j;
			rayPos.at<Point>(rays, h) = Point(x_p, y_p);
		}
	return 0;
}

/**
 * @brief viene inizializzato il kernel da applicare tramite convoluzione all'immagine normalizzata sul
 *		quale sono stati precedentemente disegnati i raggi per l'individuazione della palpebra superiore.
 *		N.B.: è qui stato utilizzato il kernel differenziale di prewitt 3x3, ma è possibile utilizzare anche sobel o scharr
 * 				scharr e sobel possono essere creati direttamente con la funzione OpenCV "getDerivKernels". Vedi la documentazione
 * 				per maggiori informazioni a riguardo.
 */
void initKernels(){
	if(horizontalK.empty()){
	/*	Mat hor;
		Mat ver;
		getDerivKernels(hor, ver, 1, 0, KSIZE, false, CV_64F);
		//horizontalK = (Mat_<double>(3,3) << -1, 0, 1, -2, 0, 2, -1, 0, 1); // sobel 3x3
		//verticalK = (Mat_<double>(3,3) << -1, -2, -1, 0, 0, 0, 1, 2, 1); // sobel 3x3*/
	/*	horizontalK = hor*ver.t();
		getDerivKernels(hor, ver, 0, 1, KSIZE, false, CV_64F);
		verticalK = hor*ver.t();*/
		horizontalK = (Mat_<double>(3,3) << 1, 0, -1, 1, 0, -1, 1, 0, -1); // prewitt 3x3
		verticalK = (Mat_<double>(3,3) << 1, 1, 1, 0, 0, 0, -1, -1, -1);	// prewitt 3x3
		
	}
	cout << "vertical = " << verticalK << endl;
	cout << "horizontal =" << horizontalK << endl;
}	


/**
 * @param normImg iride normalizzata
 * @param x
 */
double pixelConvolution(Mat* normImg, int x, int y, int ray){
	double horizontalVal = 0;
	double verticalVal = 0;
	if(ray == 0 || ray == NUM_RAYS-1){
		for(int i = -1*KSIZE/2; i <= KSIZE/2; i++){
			for(int j = 0; j <= KSIZE/2; j++){
				horizontalVal += ((int)(normImg->at<uchar>(y + j, x + i))) * horizontalK.at<double>(j+KSIZE/2,i+KSIZE/2);
				verticalVal += ((int)(normImg->at<uchar>(y + j, x + i))) * verticalK.at<double>(j+KSIZE/2,i+KSIZE/2);
			}
		}
		//return abs(horizontalVal) + abs(verticalVal); // posso ritornare sia questo che quello sotto, la differenza è minima (anche se più preciso quello sotto)
		return sqrt(pow(horizontalVal, 2) + pow(verticalVal, 2));
	}
	else{
		for(int i = -1*KSIZE/2; i <= KSIZE/2; i++){
			for(int j = -1*KSIZE/2; j <= KSIZE/2; j++){
				if((y+j) >= 0){
					horizontalVal += normImg->at<uchar>(y + j, x + i) * horizontalK.at<double>(j+KSIZE/2,i+KSIZE/2);
					verticalVal += normImg->at<uchar>(y + j, x + i) * verticalK.at<double>(j+KSIZE/2,i+KSIZE/2);
				}
			}
		}
		//return abs(horizontalVal) + abs(verticalVal);
		return sqrt(pow(horizontalVal, 2) + pow(verticalVal, 2));
	}
}


/**
 * @brief viene localizzata e individuata la palpebra superiore. Viene fatto disegnando sullo spettro rosso dell'immagine normalizzata
 *		dei raggi al quale verrà applicato, tramite convoluzione, un kernel differenziale (es: prewitt). I valori risultanti da tale 
 * 		convoluzione vengono valutati e per ogni raggio viene preso il valore massimo. Viene effettuata una scremetura di possibili outliers
 *		(questa fase sarebbe da migliorare) e viene applicata ai valori restanti la regressione polinomiale (terzo ordine).
 * 		Una volta ottenuto il polinomio di terzo ordine, vengono calcolati i valori per la curva della palpebra superiore.
 * 		Se non è possibile disegnare tale curva, si procede con la funzione ellipseFitting, in cui vengono presi anche i maxima speculari (y*-1)
 *		e viene disegnata un ellisse. Alla fine di tutto viene creata la maschera binaria di output e viene swappata per garantire una corretta
 *		posizione della palpebra inferiore e superiore per effettuare un merge di tutte le maschere.
 * @param normImg iride normalizzata
 * @param path stringa contenente il path all'interno del quale verrà salvato il file contenente l'immagine della maschera di output
 * @return reversedUpperMask conterrà la maschera di output per l'individuazione della palpebra superiore, al quale però viene effettuata
 * 		un'operazione di "swap" dei due settori della maschera, in modo tale da avere al centro superiore la posizione della palpebra inferiore
 * 		(se presente) e negli angoli in alto la palpebra superiore individuata con la presente funzione.
 */
Mat upperEyelidDetection(Mat* normImg, string path){
	Mat upper = Mat::zeros(normImg->rows, normImg->cols, CV_8UC1);	// ottengo l'iride normalizzata con al centro l'upper eyelid
	for(int ind = 0; ind < normImg->cols/2; ind++){
		for(int j = 0; j < normImg->rows; j++){
			upper.at<uchar>(j,ind) = normImg->at<uchar>(j,ind+normImg->cols/2);
		}
	}
	for(int ind = normImg->cols/2; ind < normImg->cols; ind++){
		for(int j = 0; j < normImg->rows; j++){
			upper.at<uchar>(j,ind) = normImg->at<uchar>(j,ind-normImg->cols/2);
		}
	}
	Mat upperBlur = upper.clone();
	GaussianBlur( upper, upperBlur, Size(41,41), 0, 0, BORDER_DEFAULT );
	Mat upperEyelidMask = Mat(normImg->rows, normImg->cols, CV_8UC1, Scalar(255));
	drawRays(&upper);
	initRayPos(&upper);
	initKernels();
	vector<double> vals;
	vector<uchar> maxPixels;
	int totPixelsValue = 0;
	Mat coords = Mat_<Point>(NUM_RAYS, 1);
	int index = 0;
	int totVal = 0; int count = 0;
	for(int ray = 0; ray < rayPos.rows; ray++){
		double valMax = 0; int xMax = 0; int yMax = 0;
		for(int x = 10; x < rayPos.cols-10; x++){
			double val;
			int col = rayPos.at<Point>(ray,x).x;
			int row = rayPos.at<Point>(ray,x).y;
			if( ray == 0 && col <= 330+KSIZE/2+1) continue;
			else if(ray == NUM_RAYS-1 && col >= 270-KSIZE/2-1) continue;
			if( upperBlur.at<uchar>(row,col) == 0) continue;
			if(normImg->at<uchar>(row,col) == 0 || normImg->at<uchar>(row,col-KSIZE/2) == 0 || normImg->at<uchar>(row,col+KSIZE/2) == 0) continue;
			if((ray > 0 && ray < NUM_RAYS-1) && (col == 0 || row - KSIZE/2 <= 0)) continue;
			if(rayPos.at<Point>(ray,x).y > upper.rows-10-KSIZE/2) continue;
			if( upperBlur.at<uchar>(row,col) > 150) continue;
			if(ray == 0 || ray == NUM_RAYS-1) val = pixelConvolution(&upper, rayPos.at<Point>(ray,x).x, rayPos.at<Point>(ray,x).y, ray);
			else if(ray > 0 && ray < NUM_RAYS-1) val = pixelConvolution(&upper, rayPos.at<Point>(ray,x).x, rayPos.at<Point>(ray,x).y, ray);
			if(val > valMax){valMax = val; xMax = rayPos.at<Point>(ray,x).x; yMax = rayPos.at<Point>(ray,x).y;}
		}
		coords.at<Point>(0,index++) = Point(xMax,yMax);
		if(ray > 0 && ray < NUM_RAYS-1 && valMax!=0){count++; totVal += valMax;}
		vals.push_back(valMax);
		maxPixels.push_back(upperBlur.at<uchar>(yMax,xMax));
		totPixelsValue += (int)upperBlur.at<uchar>(yMax,xMax);
	}
	int count_pix = 0;
	for(int i = 0; i < maxPixels.size(); i++){
		if(maxPixels[i] >= normImg->rows)
			count_pix++;
	}
	if(count_pix >= maxPixels.size()*0.75)
		return upperEyelidMask;

	vector<double> minimas = localMinima(vals);
	vector<Point> vecP;
	vector<int> vec_x;
	vector<int> vec_y;
	for(int i = 0; i < coords.rows; i++){
		if(find(minimas.begin(), minimas.end(), i) != minimas.end()) continue;
		Point pt = coords.at<Point>(0,i);
		if(pt.y >= upper.rows-1-KSIZE/2) continue;
		if(pt.x == 0) continue;
		vecP.push_back(pt);
		vecP.push_back(Point(pt.x, pt.y*-1));
		vec_x.push_back(pt.x);
		vec_y.push_back(pt.y);
		upperBlur.at<uchar>(pt.y, pt.x) = 255;
	}
	if(!vec_x.empty() && !vec_y.empty()){
		vector<int> indexes = getXOutliers(&vec_x, &vec_y);
	}
	vector<double> coeffs = polyfit(vec_x, vec_y, 3);
	int x1 = upper.cols/2;
	int l = (upper.cols/2)-4;
	int x_i = x1-l;
	int x_f = x1+l;
	double y = 0;
	vector<Point> polyPoints;
	int countZeros = 0;
	for(int x = x1; x >= x1-l; x--){
		y = 0;
		for(int j = 0; j < coeffs.size(); j++){
			double c = coeffs[j];
			y += c*(pow(x,j));
		}
		if(round(y) >= 0) polyPoints.push_back(Point(x,round(y)));
		if(round(y) == 0) {countZeros++; break;}
	}
	for(int x = x1; x <= x1+l; x++){
		y = 0;
		for(int j = 0; j < coeffs.size(); j++){
			double c = coeffs[j];
			y += c*(pow(x,j));
		}
		if(round(y) >= 0) polyPoints.push_back(Point(x,round(y)));
		if(round(y) == 0) {countZeros++; break;}
	}
	if(countZeros == 2){
		for(int i = 0; i < polyPoints.size(); i++){
			if(polyPoints[i].y >= 0 && polyPoints[i].y < normImg->rows){
				upperBlur.at<uchar>(polyPoints[i].y, polyPoints[i].x) = 255;
				for(int j = polyPoints[i].y; j >= 0; j--){
					upperEyelidMask.at<uchar>(j, polyPoints[i].x) = 0;
				}
			}
			else if(polyPoints[i].y >= normImg->rows){
				for(int j = normImg->rows-1; j >= 0; j--){
					upperEyelidMask.at<uchar>(j, polyPoints[i].x) = 0;
				}
			
			}
		}
	}
	else if(vecP.size() >= 10){
			RotatedRect rotRect = fitEllipse(vecP);
			ellipse(upperBlur, rotRect, Scalar(160));
			ellipse(upperEyelidMask, rotRect, Scalar(0), CV_FILLED);
	}
	Mat reversedUpperMask = Mat::zeros(normImg->rows, normImg->cols, CV_8UC1);	// adatto la maschera dell'upper eyelid (è al centro) per quella del lower eyelid (upper è agli estremi e lower al centro)
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


/*
 * @param vec
 * @return minima è un vettore di double contenente tutti i valori che non 
 */
vector<double> localMinima(vector<double> vec){
	vector<double> minima;
	double v1, v2;
	for(int i = 1; i < vec.size()-1; i++){
		v1 = vec[i-1]; v2 = vec[i+1];
		if( v1 > vec[i] && vec[i] < v2){
			cout << vec[i] << endl;
			minima.push_back(i);
		}
	}
	return minima;
}

/**
 * ritorna le posizioni degli outliers nel vettore delle X
 */
vector<int> getXOutliers(vector<int>* vec_x, vector<int>* vec_y){
	vector<int> indexes;
	int l = vec_y->size()/2;
	for(int i = 1; i < vec_x->size()-1; i++){
		if(vec_x->at(i-1) > vec_x->at(i) && vec_x->at(i) < vec_x->at(i+1)){
			indexes.push_back(vec_x->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
		else if(vec_x->at(i-1) < vec_x->at(i) && vec_x->at(i) > vec_x->at(i+1)){
			indexes.push_back(vec_x->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
	}
	for(int i = 1; i < vec_y->size()-1; i++){
		if(vec_y->at(i-1) > vec_y->at(i) && vec_y->at(i) < vec_y->at(i+1) && i > l){
			indexes.push_back(vec_y->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
		else if(vec_y->at(i-1) < vec_y->at(i) && vec_y->at(i) > vec_y->at(i+1) && i <= l){
			indexes.push_back(vec_x->at(i));
			vec_x->erase(vec_x->begin()+i);
			vec_y->erase(vec_y->begin()+i);
		}
	}
	return indexes;
}


/**
 * @brief tramite l'utilizzo della mean e standard deviation viene calcolato un valore di threshold. Tale valore viene usato per
 * 		individuare la palpebra inferiore, operazione che viene effettuata solo se la standard deviation è > della mean deviation /4
 * @param normImg spettro rosso dell'iride normalizzata
 * @return lowerEyelidMask è la maschera di individuazione della palpebra inferiore
 */
Mat lowerEyelidDetection(Mat* normImg){
	Mat lowerEyelidMask = Mat(normImg->rows, normImg->cols, CV_8UC1, Scalar(255));
	Mat meanMat, stdDevMat;
	Mat mask = Mat::zeros(normImg->rows, normImg->cols, CV_8UC1);
	for(int y = 0; y <= normImg->rows/2; y++){
		for(int x = normImg->cols/4; x <= (3*normImg->cols)/4; x++){
			mask.at<int>(y,x) = 1;
		}
	}
	meanStdDev(*normImg, meanMat, stdDevMat, mask);
	double stdDev = stdDevMat.at<double>(0,0);
	double mean = meanMat.at<double>(0,0);
	int threshold = (int)(mean + stdDev);
	cout << "mean = " << mean << ", stdDev = " << stdDev << ", threshold = " << threshold << endl;
	if(stdDev > mean/4){
		//uchar threshold = (uchar)(mean + stdDev/2);
		for(int y = 0; y < normImg->rows; y++){
			for(int x = 0; x < normImg->cols; x++){
				if(normImg->at<uchar>(y,x) > threshold)
					lowerEyelidMask.at<uchar>(y,x) = 0;
			}
		}
	}
	return lowerEyelidMask;
}


/**
 * @brief tramite adaptive thresholding vengono individuati i riflessi all'interno dell'immagine normalizzata (spettro blu)
 * @param normImg spettro blu dell'iride normalizzata
 * @param ksize grandezza della finestra per l'applicazione dell'adaptive threshold
 * @param c coefficiente sommato nel calcolo del threshold nella funzione OpenCV di adaptive thresholding
 * @return reflectionMask è la maschera dei riflessi individuati
 */
Mat threshReflectionDetection(Mat* normImg, int ksize, double c){
	Mat reflectionMask;
	adaptiveThreshold(*normImg, reflectionMask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, ksize, c);
	return reflectionMask;
}

// ad ogni theta io calcolo anche i punti interni al segmento disegnato, in questo modo 
// andrò a diminuire sempre di più l (l--) fino a che l = 0 ( e in quel caso sarei su y = 0  e x = cols/2)
// fatto questo non mi resta che provare a valutare tutti i punti sul raggio per quel determinato angolo (theta) e
// valutarne la convoluzione (di tutti i punti sul raggio) e ottenere alla fine il maxima.