#include "reflection_correction.h"

/**
 * @brief Diminuisco width (cols number) dell'immagine input per farla arrivare ad un valore poco >= 200. Della stessa quantità diminuisco anche l'height (rows number).
 * @param input immagine di input
 * @param output immagine di output
 */
void resize_image(Mat* input, Mat* output){
	int quotient = input->cols / 250;
	resize(*input, *output, Size(input->cols/quotient, input->rows/quotient));
}


/**
 * @brief Applico un adaptive threshold sul canale blu dell'immagine di input (perché i riflessi sono molto più visibili in questo canale).
 * 		Sull'output dell'adaptive threshold, applico il dilate, funzione utile per garantire una corretta copertura dei riflessi.
 * 		In output, alla fine di tale metodo, avrò una maschera binaria contenente i riflessi individuati dell'immagine d'input.
 * @param input immagine di input
 * @param mask immagine di output (maschera vuota)
 * @param ksize grandezza del kernel (ksize x ksize)
 * @param c coefficiente da sottrarre alla media gaussiana pesata
 */
void search_reflection(Mat* input, Mat* mask, int ksize, double c){
	Mat bgr[3];
	split(*input, bgr);
	adaptiveThreshold(bgr[0], *mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, ksize, c);
	dilate(*mask, *mask, Mat(), Point(-1,-1), 2);
}



/**
 * @brief Applico l'algoritmo di inpainting [TELEA] con maschera "mask", immagine di input "input" e output "output". Tale algoritmo è applicato un numero di volte pari a iterations.
 * @param input immagine di input (ottenuta dall'output di una precedente chiamata di resize_image)
 * @param mask immagine di mask (ottenuta da una precedente chiamata di search_reflection)
 * @param output immagine di output
 * @param iterations quantità di volte in cui viene applicato l'algoritmo di inpainting
 */
void inpaint_reflection(Mat* input, Mat* mask, Mat* output, int iterations){
	inpaint(*input, *mask, *output, iterations, INPAINT_TELEA);
}