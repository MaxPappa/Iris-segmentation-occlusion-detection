#include "normalization.h"

/**
 * @brief Normalizzazione secondo la normalizzazione ideata da Daugman: Homogeneous Rubber Sheet Model.
 * 		Viene inoltre effettuata una mappatura del tipo (cart.x,cart.y) = (pol.x, pol.y) dove cart equivale alle coordinate cartesiane
 * 		mentre pol sono le coordinate polari.
 * @param inp_img immagine sul quale sono state individuate iride e pupilla
 * @param irisR raggio dell'iride individuata
 * @param irisX coordinata X della posizione centrale dell'iride individuata
 * @param irisY coordinata Y della posizione centrale dell'iride individuata
 * @param pupilR raggio della pupilla individuata
 * @param pupilX coordinata X della posizione centrale della pupilla individuata
 * @param pupilY coordinata Y della posizione centrale della pupilla individuata
 * @param mappa mappa utilizzata per effettuare una mappatura da coordinate cartesiane a coordinate polari. Questa mappatura 
 * 	verrà usata successivamente per fare una trasformazione da coordinate polari a cartesiane della maschera dei difetti finale dell'algoritmo
 */
Mat normalizza(Mat* inp_img, int irisR, int irisX, int irisY, int pupilR, int pupilX, int pupilY, map<string, Point>* mappa){
	int height = irisR*2;
	int width = round(irisR * 2 * M_PI);
	Mat out_img = Mat::zeros(height, width, CV_8UC3);
	double theta_step = (2*M_PI)/width;
	float xp, yp, xi, yi;
	int x, y;
	int ind = 0;
	for(double i = 3*M_PI/2; i < 2*M_PI + 3*M_PI/2; i += theta_step){
		xp = (pupilX + pupilR * cos(i));
		yp = (pupilY + pupilR * sin(i));
		xi = (irisX + irisR * cos(i));
		yi = (irisY + irisR * sin(i));
		for(int j = 0; j < height; j++){
			float pas = (float)j/height;
			int x = round( ( 1 - pas ) * xi + pas * xp );
			int y = round( ( 1 - pas ) * yi + pas * yp );
			if ( x >= 0 && x < inp_img->cols && y >= 0 && y < inp_img->rows ){
				out_img.at<Vec3b>(j,ind) = inp_img->at<Vec3b>(y,x);		// coordinate per avere upper eyelid al centro
				mappa->insert(pair<string,Point>(to_string(ind)+","+to_string(j), Point(x,y)));
			}
		}
		ind++;
	}
	return out_img;
}