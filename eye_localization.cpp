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