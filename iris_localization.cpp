#include "iris_localization.h"

/** 
 * @brief stampa un vettore/matrice
 * @param v singolo valore di un vettore/matrice
 */
template<typename T>
void print(std::vector<T> const &v)
{
    for (auto i: v) {
        std::cout << i << ' ';
    }
    std::cout << '\n';
}


/**
 * @brief ritorna la quantità di pixel presenti sulla circonferenza del cerchio con raggio r, tale valore è utilizzabile per ottenere il corretto valore per cui dividere 2*PI
 * 		in modo tale da evitare, per raggi piccoli, divisioni enormi (e.g. 360) per la quale uno stesso valore verrebbe sommato più e più volte
 * @param r raggio preso in considerazione
 * @return quantità di pixel presenti sulla circonferenza del cerchio con raggio r
 */
int getAmountCirclePixels(int r){
	return 4*round(2*((1/(sqrt(2)) * r)));
}


/**
 * @brief ottengo la porzione utilizzabile del vettore contenente gli integrali lineari (per diversi raggi di un dato centro c)
 * @param v vettore dal quale prendere una slice
 * @param m indice dal quale iniziare a prendere la slice
 * @param n indice al quale termina la slice
 * @return porzione del vettore d'input, che inizia dall'indice m e finisce all'indice n
 */
template<typename T>
std::vector<T> slice(std::vector<T> &v, int m, int n)
{
    std::vector<T> vec;
    std::copy(v.begin() + m, v.begin() + n + 1, std::back_inserter(vec));
    return vec;
}


/**
 * @brief ottengo e ritorno l'indice del primo valore che nell'integrale lineare è pari a 0
 * @param line_int vettore contenente i valori dell'integrale lineare per ogni singolo raggio preso in considerazione per un centro(x,y)
 * @return indice del vettore d'input il cui valore è 0, oppure -1 se non è presente nessuno 0
 */
int getIndexOfZeros(vector<int> line_int){
	for( int i = 0; i < line_int.size(); i++ ){
		if( line_int[i] == 0 ) return i;
	}
	return -1;
}


/**
 * @brief calcolo il valore del pixel preso in considerazione (x,y) secondo la modalità LUA (somma pesata per percezione)
 * @param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 * @param angle attuale angolo della circonferenza (presa in considerazione)
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
 * @return somma pesata del pixel sui tre canali BGR, secondo la formula B[pixel]*0.114 + G[pixel]*0.587 + R[pixel]*0.299
 */
uchar pixel_value(image* img, double angle, Point centro, int r){
	int x = r * cos(angle) + centro.x;
	int y = r * sin(angle) + centro.y;
	uchar value_blue = img->blue->at<uchar>(y,x) * 0.114; // * 0.114
	uchar value_green = img->green->at<uchar>(y,x) * 0.587; // *0.587
	uchar value_red = img->red->at<uchar>(y,x) * 0.299; // *0.299
	//uchar value = img->at<uchar>(y,x);
	return value_blue+value_green+value_red;
}


/**
 * @brief calcolo la somma dei valori presenti sui punti della circonferenza
 * @param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
 * @return somma di valori del perimetro della circonferenza presa in considerazione n.b.: questa somma rappresenta l'integrale lineare chiuso secondo la formula di Daugman discretizzata
 */
int contour_sum(image* img, Point centro, int r){	// calcolo il contorno della circonferenza che va' da 0° a 45°, e da 135° a 360° (evito una probabile palpebra superiore)
	int sum = 0;
	int porzioni = getAmountCirclePixels(r);
	double theta = (2*M_PI)/porzioni; // piccola porzione di 2*pi
	for(double angle = 0; angle <= 2*M_PI/8; angle+=theta)	// da 0° a 45°
	{
		sum += pixel_value(img, angle, centro, r);
	}
	for(double angle = (2*M_PI*3)/8; angle <= (2*M_PI*5)/8; angle+=theta){ sum += pixel_value(img, angle, centro, r); }
	/*for(double angle = (2*M_PI*3)/8; angle <= 2*M_PI; angle+=theta)	// da 135° a 360°
	{
		sum += pixel_value(img, angle, centro, r);
	}*/
	for(double angle = (2*M_PI*7)/8; angle <= 2*M_PI; angle+=theta){ sum += pixel_value(img, angle, centro, r); }
	return sum;
}


/**
 * @brief calcolo l'integrale di linea dei valori con posizione sulla circonferenza del cerchio di raggio r, con r appartenente a radius_range
 * @param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param radius_range vettore contenente i raggi presi in considerazione ( da r_min a r_max )
 * @return line_integral vettore contenente un integrale di linea per ogni raggio presente in radius_range, integrale di linea sulla circonferenza con origine pari a centro
 */
vector<int> linear_integral_vector(image* img, Point centro, vector<int> radius_range){
	vector<int> line_integral;
	for( int i = 0; i < radius_range.size(); i++ ){					// ignoro l'arco superiore ( > 45° fino a < 135° ) poiché non viene considerato all'interno del calcolo di integrale lineare
		//int p1_y = radius_range[i] * sin(2*M_PI/8) + centro.y;		// quindi prendo le coordinate y estreme ( a 45° e a 135° ) che mi consentono di calcolare l'integrale anche se la circonferenza
		//int p2_y = radius_range[i] * sin((2*M_PI*3)/8) + centro.y;	// non include l'arco superiore nell'immagine
		if( centro.x - radius_range[i] < 0 || centro.x + radius_range[i] > img->cols || centro.y + radius_range[i] > img->rows || (centro.y - radius_range[i]) < 0 ) // oppure  p1_y < 0 || p2_y < 0
		{
			line_integral.push_back(0);
		}
		else{
			int tmp = contour_sum(img, centro, radius_range[i]);
			line_integral.push_back(tmp);
		}
	}
	return line_integral;
}


/**
 * @brief integrale di linea dei valori con posizione sulla circonferenza del cerchio di raggio r, con r appartenente a radius_range
 * @see linear_integral_vector la differenza è che per la pupilla tengo in considerazione, e calcolo l'integrale, per l'intera circonferenza
 * @param img_red matrice del canale RED dell'immagine di input. Utilizzo solo lo spettro di colore rosso perché permette una migliore visione della pupilla
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param radius_range vettore contenente i raggi presi in considerazione ( da r_min a r_max )
 * @return line_integral vettore contenente un integrale di linea per ogni raggio presente in radius_range, integrale di linea sulla circonferenza con origine pari a centro
 */
vector<int> pupil_linear_integral_vector(Mat* img_red, Point centro, vector<int> radius_range){
	vector<int> line_integral;
	for( int i = 0; i < radius_range.size(); i++ ){
		if( centro.x - radius_range[i] < 0 || centro.x + radius_range[i] > img_red->cols || centro.y + radius_range[i] > img_red->rows || centro.y - radius_range[i] < 0)
		{
			line_integral.push_back(0);
		}
		else{
			int tmp = pupil_contour_sum(img_red, centro, radius_range[i]);
			line_integral.push_back(tmp);
		}
	}
	return line_integral;
}


/**
 * @brief calcolo la somma dei valori dei pixel sulla circonferenza di con punto centrale centro e raggio r
 * @see contour_sum la differenza è che per la pupilla calcolo l'integrale discretizzato ( somma dei pixel sul perimetro della circonferenza ) per l'intera circonferenza, perché
 *		si suppone non ci siano occlusioni dovute a ciglia e/o palpebra superiore
 * @param img_red matrice del canale RED dell'immagine di input
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
 * @return somma dei valori dei pixel sulla circonferenza
 */
int pupil_contour_sum(Mat* img_red, Point centro, int r){
	int sum = 0;
	int porzioni = getAmountCirclePixels(r);
	double theta = (2*M_PI)/porzioni; // piccola porzione di 2*pi
	for(double angle = 0; angle <= 2*M_PI; angle+=theta)	// per la pupilla prendo l'intera circonferenza
	{
		sum += pupil_pixel_value(img_red, angle, centro, r);
	}
	return sum;
}


/**
 * @brief calcolo le coordinate del pixel (x,y) usando angolo, centro e raggio dela circonferenza per ottenere il valore del pixel
 * @see pixel_value la differenza è che per la pupilla, avendo un singolo canale (il rosso) non faccio nessuna somma pesata
 * @param img_red matrice del canale RED dell'immagine di input
 * @param angle angolo attualmente preso in considerazione, viene utilizzato nel calcolo delle coordinate del punto (x,y) all'angolo angle della circonferenza presa in considerazione
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
 * @return valore del pixel (x,y) sulla circonferenza della pupilla con raggio r
 */
uchar pupil_pixel_value(Mat* img_red, double angle, Point centro, int r){
	int x = r * cos(angle) + centro.x;
	int y = r * sin(angle) + centro.y;
	uchar value = img_red->at<uchar>(y,x);
	return value;
}


/**
 * @brief calcola valori di width e height in modo da utilizzarli per ridimensionare le immagini e rendere la computazione dell'algoritmo quanto più veloce possibile
 * @param cols colonne (width) della matrice (immagine) presa in considerazione
 * @param rows righe (height) della matrice (immagine) presa in considerazione
 * @return pair<int,int> contenente cols/k e rows/k, k è scelto in base al valore che più si avvicina al quoziente colonne/256 in modo da poter ridimensionare tutte le immagini 
 *		allo stesso modo e con width <= 256 (stesso discorso vale per l'height, ammesso che cols>=rows)
 */
pair<int,int> obtain_w_h(int cols, int rows){
	if(cols/256 <= 0) return pair<int,int>(0,0);
	int h = cols/256;
	int k = h;
	while(cols%k != 0 || rows%k != 0){
		k--;
	}
	pair<int,int> coppia(cols/k, rows/k);
	return coppia;
}


/**
 * @brief calcolo i valori di width e height da utilizzare per ridimensionare l'immagine (appartenente a MICHE)
 * @param cols colonne (width) della matrice (immagine) presa in considerazione
 * @param rows righe (height) della matrice (immagine) presa in considerazione
 * @return pair<int,int> contenente cols/k e rows/k
 */
pair<int,int> obtain_w_h_miche(int cols, int rows){
	int k = cols;
	int count = 1;
	bool flag = false; bool next = true;
	while((k > 42 || flag) && next){
		for(int i = 2; i <= k; i++){
			if(k%i == 0){
				if(k/i > 42)
					k = k/i;
				else next = false;
				break;
			}
			else if(i == k) flag = true;
		}
	}
	pair<int,int> coppia(k,k);
	return coppia;
}

/**
 * @brief ricerca di k, valore che verrà successivamente valutato per capire se è possibile o meno ridimensionare l'immagine con un coefficiente che non modifichi di height e width rendendo l'immagine rumorosa
 * @param width numero di colonne (larghezza) dell'immagine
 * @return intero k utilizzato per valutare se allargare o diminuire la lunghezza dell'immagine, in modo da avere un coefficiente con cui dividere equamente width e height senza stravolgere l'immagine 
 */
int checkWidth(int width){
	int k = width;
	int count = 1;
	bool flag = false; bool next = true;
	while((k > 42 || flag) && next){
		for(int i = 2; i <= k; i++){
			if(k%i == 0){
				if(k/i > 42)
					k = k/i;
				else next = false;
				break;
			}
			else if(i == k) flag = true;
		}
	}
	return k;
}

/**
 * @brief applicazione dell'operatore integro-differenziale di Daugman (vedi Krupicka e Daugman) in modalità LUA discretizzato per ottenere il contorno dell'iride (limbus)
 * @param img struct contenente matrici dei 3 canali (BGR), numero di colonne, numero di righe (width e height)
 * @param r_min raggio minimo preso in considerazione. Di norma equivale a height/4
 * @param r_max raggio massimo preso in considerazione. Di norma equivale a height/2
 * @return results struct contenente raggio, punto centrale (x,y) e valore massimo. Il valore massimo equivale al valore massimo che l'operatore integro-differenziale 
 * 		ottiene in base ai vari raggi e punti centrali presi in considerazione
 */
results* apply_daugman_operator(image* img, int r_min, int r_max){
	double sigma = SIGMA;
	int rows = img->rows;
	int cols = img->cols;
	results* res = (results*) calloc(1, sizeof(results));
	/*r_max = r_max + (DELTA_R/2);		// così da usare tutti i raggi, altrimenti andrei a salter gli ultimi DELTA_R/2 raggi
	r_min = r_min - (DELTA_R/2) -1;		// così da usare tutti i raggi, altrimenti andrei a salter i primi DELTA_R/2 -1 raggi*/
	vector<int> radius_range(r_max-r_min+1);
	std::iota(begin(radius_range), end(radius_range), r_min);	// inserisco in radius_range r_max-r_min valori, partendo da r_min e incrementando di 1 ogni volta che inserisco	
	double max = 0;
	Mat convolved;
	for( int y = 0; y < rows; y++ ){
		for( int x = 0; x < cols; x++ ){
			Point centro(x,y);
			vector<int> line_int = linear_integral_vector(img, centro, radius_range);	// applico, per ogni raggio appartenente a radius_range, l'integrale lineare al cerchio con centro x,y
			if( find(line_int.begin(), line_int.end(), 0) != line_int.end() ){
				int addr = getIndexOfZeros(line_int);	// ottengo l'indirizzo in line_int che contiene il primo zero. Serve per ottenere una slice utilizzabile di line_int, perché è possibile che
														// per alcuni raggi il cerchio con centro x,y esca fuori, ma questo non sempre vale per tutti i raggi con tale centro
				if( addr == 0 || addr < DELTA_R +1 ) continue; 	// se l'indirizzo è minore di DELTA_R+1, significa che ci sono meno di DELTA_R+1 valori utilizzabili in line_int, e quindi
															  	// il kernel non può scorrere per n-k e/o n-k-1
				vector<int> new_line_int = slice(line_int, 0, addr);	// ottengo la slice utilizzabile
				convolved = convolution(sigma, &new_line_int);		// applico la convoluzione
				double val = 0;
				// in convolved ho n valori quanti sono i raggi, ma gli ultimi (delta_r/2)*2 non sono utilizzabili (all'indice 0 corrisponde il primo elemento utile, a (delta_r/2)*2-1 l'ultimo utile)
				// perché non prendo i raggi troppo a sinistra e troppo a destra (dove non posso applicare la convoluzione con kernel
				for( int ind = 0; ind < convolved.cols-((DELTA_R/2)*2)-1; ind++ ){
					val = abs(convolved.at<double>(0, ind) - convolved.at<double>(1, ind))/DELTA_R;	// per ogni valore in convoluzione, faccio abs(conv(n-k) - conv(n-k-1))/delta_r
					if( val > max ){		// sostituisco il valore ( e lo valuto come possibile candidato limbus ) solo se è quello con valore maggiore (vedi operatore di Daugman)
						max = val;
						res->radius = radius_range[ind+(DELTA_R/2)+1];
						res->center = centro;
						res->value = val;
					}
				}
			}
			else{	// se non ci sono 0 all'interno di line_int significa che tutti i valori sono utilizzabili, e quindi per ogni cerchio con raggio in radius_range e centro x,y non c'è alcun problema 
				convolved = convolution(sigma, &line_int);	// applico la convoluzione
				double val = 0;
				// in convolved ho n valori quanti sono i raggi, ma gli ultimi (delta_r/2)*2 non sono utilizzabili (all'indice 0 corrisponde il primo elemento utile, a (delta_r/2)*2-1 l'ultimo utile)
				// perché non prendo i raggi troppo a sinistra e troppo a destra (dove non posso applicare la convoluzione con kernel
				for( int ind = 0; ind < convolved.cols-((DELTA_R/2)*2)-1; ind++ ){
					val = abs(convolved.at<double>(0, ind) - convolved.at<double>(1, ind))/DELTA_R;	// per ogni valore in convoluzione, faccio abs(conv(n-k) - conv(n-k-1))/delta_r
					if( val > max ){		// sostituisco il valore ( e lo valuto come possibile candidato limbus ) solo se è quello con valore maggiore (vedi operatore di Daugman)
						max = val;
						res->radius = radius_range[ind+(DELTA_R/2)+1];
						res->center = centro;
						res->value = val;
					}
				}
			}
		}
	}
	return res;
}


/**
 * @brief convoluzione tra kernel gaussiano (di dimensione DELTA_R e coefficiente sigma) e vettore dell'integrale lineare 
 *		N.B.: ogni elemento corrisponde ad un integrale di linea con un determinato raggio, il punto centrale preso in considerazione è lo stesso per tutti i valori del vettore
 * @param sigma coefficiente per il calcolo del gaussian kernel
 * @param line_int vettore contenente l'integrale lineare per tutti i raggi (r_min fino a r_max, con indice crescente corrisponde un raggio maggiore del precedente) per un determinato punto centrale
 * @return matrice a due righe ed n colonne, ogni colonna corrisponde a un valore di convoluzione dell'integrale lineare con kernel gaussiano, convoluzione applicata con punto centrale del kernel
 *		sul valore in posizione pos del vettore dell'integrale lineare. La prima riga è per n-k (vedi Daugman discretizzato), la seconda riga è per n-k-1
 */
Mat convolution(double sigma, vector<int>* line_int){
	vector<double> kernel = getGaussianKernel(DELTA_R, sigma);	// creo il 1-D gaussian Kernel. La funzione ritorna un DELTA_R X 1 Mat, ma facendo cast a vector, diventa vetctor 1xDELTA_R
	int li_size = line_int->size();
	Mat target(2, li_size, CV_64F);
	int l = 0;
	double value;
	for( int pos = DELTA_R/2+1; pos < li_size-DELTA_R/2; pos++ ){// non prendo come centro i raggi che farebbero andare troppo a sinistra o troppo a destra i valori del kernel (i lati escono dal range)
		value = pixel_conv(line_int, &kernel, pos);
		target.at<double>(0,l) = value;
		
		value = pixel_conv(line_int, &kernel, pos-1);
		target.at<double>(1,l) = value;
		
		l++;
	}
	return target;
}


/**
 * @brief applicazione dell'integro-differential operator di Daugman (non modificato, vedi Daugman) discretizzato per ottenere il contorno della pupilla
 * @param img_red matrice del canale RED dell'immagine di input
 * @param r_min raggio minimo preso in considerazione. Per la pupilla è pari al raggio dell'iride ottenuta diviso per 5
 * @param r_max raggio massimo preso in considerazione. Per la pupilla è pari al raggio dell'iride ottenuta moltiplicato per 0.75
 * @return results struct contenente raggio, punto centrale (x,y) e valore massimo. Il valore massimo equivale al valore massimo che l'operatore integro-differenziale 
 * 		ottiene in base ai vari raggi e punti centrali presi in considerazione
 */
results* pupil_daugman_operator(Mat* img_red, int r_min, int r_max){
	double sigma = SIGMA;
	int rows = img_red->rows;
	int cols = img_red->cols;
	results* res = (results*) calloc(1, sizeof(results));
	vector<int> radius_range(r_max-r_min+1);
	std::iota(begin(radius_range), end(radius_range), r_min);	// inserisco in radius_range r_max-r_min valori, partendo da r_min e incrementando di 1 ogni volta che inserisco
	double max = 0;
	Mat convolved;
	for( int y = 0; y < rows; y++ ){
		for( int x = 0; x < cols; x++ ){
			if( y < (rows/2 - rows/4) || y > (rows/2 + rows/4) ) continue;
			if( x < (cols/2 - cols/4) || x > (cols/2 + cols/4) ) continue;
			Point centro(x,y);
			vector<int> line_int = pupil_linear_integral_vector(img_red, centro, radius_range);	// per ogni raggio appartenente a radius_range applico l'integrale (per pupilla) al cerchio con centro x,y
			if( find(line_int.begin(), line_int.end(), 0) != line_int.end() ){
				int addr = getIndexOfZeros(line_int);	// ottengo l'indirizzo in line_int che contiene il primo zero. Serve per ottenere una slice utilizzabile di line_int, perché è possibile che
														// per alcuni raggi il cerchio con centro x,y esca fuori, ma questo non sempre vale per tutti i raggi con tale centro
				if( addr == 0 || addr < DELTA_PUP +1 ) continue;	// se l'indirizzo è minore di DELTA_R+1, significa che ci sono meno di DELTA_R+1 valori utilizzabili in line_int, e quindi
															  	// il kernel non può scorrere per n-k e/o n-k-1
				vector<int> new_line_int = slice(line_int, 0, addr);	// ottengo la slice utilizzabile
				convolved = pup_convolution(img_red, sigma, &new_line_int, radius_range, centro);		// applico la convoluzione per pupilla
				double val = 0;
				// in convolved ho n valori quanti sono i raggi, ma gli ultimi (delta_r/2)*2 non sono utilizzabili (all'indice 0 corrisponde il primo elemento utile, a (delta_r/2)*2-1 l'ultimo utile)
				// perché non prendo i raggi troppo a sinistra e troppo a destra (dove non posso applicare la convoluzione con kernel
				for( int ind = 0; ind < convolved.cols-((DELTA_PUP/2)*2)-1; ind++ ){
					val = abs(convolved.at<double>(0, ind) - convolved.at<double>(1, ind))/DELTA_R;	// per ogni valore in convoluzione, faccio abs(conv(n-k) - conv(n-k-1))/delta_r
					if( val > max ){		// sostituisco il valore ( e lo valuto come possibile candidato limbus ) solo se è quello con valore maggiore (vedi operatore di Daugman)
						max = val;
						res->radius = radius_range[ind+(DELTA_PUP/2)+1];
						res->center = centro;
						res->value = val;
					}
				}
			}
			else{	// se non ci sono 0 all'interno di line_int significa che tutti i valori sono utilizzabili, e quindi per ogni cerchio con raggio in radius_range e centro x,y non c'è alcun problema
				convolved = pup_convolution(img_red, sigma, &line_int, radius_range, centro);	// applico la convoluzione per pupilla
				double val = 0;
				// in convolved ho n valori quanti sono i raggi, ma gli ultimi (delta_r/2)*2 non sono utilizzabili (all'indice 0 corrisponde il primo elemento utile, a (delta_r/2)*2-1 l'ultimo utile)
				// perché non prendo i raggi troppo a sinistra e troppo a destra (dove non posso applicare la convoluzione con kernel
				for( int ind = 0; ind < convolved.cols-((DELTA_PUP/2)*2)-1; ind++ ){
					val = abs(convolved.at<double>(0, ind) - convolved.at<double>(1, ind))/DELTA_R;	// per ogni valore in convoluzione, faccio abs(conv(n-k) - conv(n-k-1))/delta_r
					if( val > max ){		// sostituisco il valore ( e lo valuto come possibile candidato limbus ) solo se è quello con valore maggiore (vedi operatore di Daugman)
						max = val;
						res->radius = radius_range[ind+(DELTA_PUP/2)+1];
						res->center = centro;
						res->value = val;
					}
				}
			}
		}
	}
	return res;
}


/**
 * @brief convoluzione tra kernel gaussiano (di dimensione DELTA_R e coefficiente sigma) e vettore dell'integrale lineare 
 *		N.B.: ogni elemento corrisponde ad un integrale di linea con un determinato raggio, il punto centrale preso in considerazione è lo stesso per tutti i valori del vettore
 *		inoltre, a differenza di convolution, divido ogni elemento uscente dal calcolo della convoluzione con il valore (somma) dei pixel sul perimetro della ciconferenza con raggio r-2
 *		dove r è il raggio preso in considerazione come punto centrale della convoluzione
 * @param img_red matrice del canalre rosso (BGR) dell'immagine presa in considerazione
 * @param sigma coefficiente per il calcolo del gaussian kernel
 * @param line_in vettore contenente l'integrale lineare per tutti i raggi (r_min fino a r_max, con indice crescente corrisponde un raggio maggiore del precedente) per un determinato punto centrale
 * @param radius_range vettore contenente i raggi presi in considerazione ( da r_min a r_max )
 * @return matrice a due righe ed n colonne, ogni colonna corrisponde a un valore di convoluzione dell'integrale lineare con kernel gaussiano, convoluzione applicata con punto centrale del kernel
 *		sul valore in posizione pos del vettore dell'integrale lineare. La prima riga è per n-k (vedi Daugman discretizzato), la seconda riga è per n-k-1
 */
Mat pup_convolution(Mat* img_red, double sigma, vector<int>* line_int, vector<int> radius_range, Point centro){
	vector<double> kernel = getGaussianKernel(DELTA_PUP, sigma);	// creo il 1-D gaussian Kernel. La funzione ritorna un DELTA_R X 1 Mat, ma facendo cast a vector, diventa vetctor 1xDELTA_R
	int li_size = line_int->size();
	Mat target(2, li_size, CV_64F);
	int l = 0;
	double value; int divider;
	for( int pos = DELTA_PUP/2+1; pos < li_size-DELTA_PUP/2; pos++ ){// non prendo come centro i raggi che farebbero andare troppo a sinistra o troppo a destra i valori del kernel (i lati escono dal range)
		value = pixel_conv(line_int, &kernel, pos);
		divider = pup_contour_divider(img_red, radius_range, pos-2, centro);
		value = value / ((double)divider);
		target.at<double>(0,l) = value;
		
		value = pixel_conv(line_int, &kernel, pos-1);
		value = value / ((double)divider);
		target.at<double>(1,l) = value;
		
		l++;
	}
	return target;
}

/**
 * @brief pup_contour_divider ritorna la somma dei pixel sul perimetro della circonferenza con centro(x,y) "centro" e raggio radius_range[pos-2] dove pos è la posizione del kernel sul vettore
 * 		degli integrali di linea. Per maggiori dettagli, vedi pup_convolution e l'operatore di Daugman discretizzato, in particolare l'operatore discretizzato modificato per il calcolo della pupilla
 * @param img_red matrice del canalre rosso (BGR) dell'immagine presa in considerazione
 * @param radius_range vettore contenente i raggi presi in considerazione ( da r_min a r_max )
 * @param pos valore centrale del kernel della convoluzione. N.B.: questa pup_contour_divider viene chiamato SOLO da pup_convolution, che gli da' in input pos. pos è necessario per ottenere il raggio r-2
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @return ritorna la somma dei pixel presenti sul perimetro della circonferenza presa in considerazione
 */
int pup_contour_divider(Mat* img_red, vector<int> radius_range, int pos, Point centro){
	int sum = 0; int r = radius_range[pos];
	int porzioni = getAmountCirclePixels(r);
	double theta = (2*M_PI)/porzioni;
	int x; int y;
	for(double angle = 0; angle <= 2*M_PI; angle+=theta){
		x = r * cos(angle) + centro.x;
		y = r * sin(angle) + centro.y;
		sum += img_red->at<uchar>(y,x);
	}
	return sum;
}

/**
 * @brief con pixel_conv, ed in particolare con pixel, qui si intende il singolo elemento del vettore line_int su cui viene fatta convoluzione con il kernel gaussiano
 * @param line_in vettore contenente l'integrale lineare per tutti i raggi (r_min fino a r_max, con indice crescente corrisponde un raggio maggiore del precedente) per un determinato punto centrale
 * @param kernel kernel gaussiano da applicare al vettore line_int
 * @param pos posizione dell'elemento in line_int su cui l'elemento centrale del kernel si sovrappone
 * @return valore ottenuto dalla convoluzione tra il kernel gaussiano e gli elementi del vettore degli integrali sul quale il kernel si sovrappone
 */
double pixel_conv(vector<int>* line_int, vector<double>* kernel, int pos){
	double val = 0;
	int l = 0;
	int mean = kernel->size() / 2;	// prendo la metà parte intera inferiore del kernel
	int h = pos + mean;
	for( int j = 0; j < kernel->size(); j++ ){
		val += ((double) line_int->at(h)) * kernel->at(j);
		h--;
	}
	return val;
}

/**
 * @brief viene creata una maschera binaria (coordinate cartesiane) per l'iride e pupilla individuate sull'immagine di input dell'algoritmo
 * @param src_image immagine da utilizzare per creare la maschera binaria
 * @param iris_r raggio dell'iride individuata
 * @param iris_c centro dell'iride individuata
 * @param pupil_r raggio della pupilla individuata
 * @param pupil_c centro della pupilla individuata
 * @return inizializzazione della maschera binaria dell'immagine originale (non normalizzata). Inizializzata valutando tutti i pixel tra iride e pupilla.
 */
Mat binaryMask(Mat* src_image, int iris_r, Point iris_c, int pupil_r, Point pupil_c){
	Mat mask = Mat::zeros(Size(src_image->cols, src_image->rows), CV_8UC1);
	circle(mask, iris_c, iris_r, Scalar(255), -1); // thickness = -1 -> filled circle
	circle(mask, pupil_c, pupil_r, Scalar(0), -1);
	return mask;
}


/**
 * @brief split per string
 * @param strToSplit stringa sul quale effettuare lo split
 * @param delimiter delimitatore usato per effettuare lo split
 * @return vector<string> contenente gli elementi della stringa al quale è stato effettuato lo split
 */
vector<string> split(string strToSplit, char delimeter)
{
    stringstream ss(strToSplit);
    string item;
    vector<string> splittedStrings;
    while (std::getline(ss, item, delimeter))
    {
       splittedStrings.push_back(item);
    }
    return splittedStrings;
}


// da qui in poi è per la multi convoluzione

/**
 * @brief ritorno il valore del pixel preso in considerazione (x,y) per lo spettro da considerare (col)
 * @param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 * @param angle attuale angolo della circonferenza (presa in considerazione)
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
 * @param col spettro dell'immagine da prendere in considerazione ("blue", "green", "red")
 * @return valore del pixel (x,y) dello spettro col
 */
uchar pixel_value_multi(image* img, double angle, Point centro, int r, string col){
	int x = r * cos(angle) + centro.x;
	int y = r * sin(angle) + centro.y;
	if(col == "blue")
		return img->blue->at<uchar>(y,x); 
	else if(col == "green")
		return img->green->at<uchar>(y,x); 
	else if(col == "red")
		return img->red->at<uchar>(y,x);
}

/**
 * @brief calcolo la somma dei valori presenti sui punti della circonferenza
 * @param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
 * @param col colore dello spettro da utilizzare nell'immagine img
 * @return somma di valori del perimetro della circonferenza presa in considerazione n.b.: questa somma rappresenta l'integrale lineare chiuso secondo la formula di Daugman discretizzata
 */
int contour_sum_multi(image* img, Point centro, int r, string col){	// calcolo il contorno della circonferenza che va' da 0° a 45°, e da 135° a 360° (evito una probabile palpebra superiore)
	int sum = 0;
	int porzioni = getAmountCirclePixels(r);
	double theta = (2*M_PI)/porzioni; // piccola porzione di 2*pi
	for(double angle = 0; angle <= 2*M_PI/8; angle+=theta)	// da 0° a 45°
	{
		sum += pixel_value_multi(img, angle, centro, r, col);
	}
	for(double angle = (2*M_PI*3)/8; angle <= (2*M_PI*5)/8; angle+=theta){ sum += pixel_value_multi(img, angle, centro, r, col); }
	/*for(double angle = (2*M_PI*3)/8; angle <= 2*M_PI; angle+=theta)	// da 135° a 360°
	{
		sum += pixel_value(img, angle, centro, r);
	}*/
	for(double angle = (2*M_PI*7)/8; angle <= 2*M_PI; angle+=theta){ sum += pixel_value_multi(img, angle, centro, r, col); }
	return sum;
}

/**
 * @brief integrale di linea dei valori con posizione sulla circonferenza del cerchio di raggio r, con r appartenente a radius_range. Modalità multi-spettro, per cui calcolo l'integrale su singolo spettro.
 * @param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param radius_range vettore contenente i raggi presi in considerazione ( da r_min a r_max )
 * @param col colore ("red", "blue", "green") dello spettro utilizzato
 * @return line_integral vettore contenente un integrale di linea per ogni raggio presente in radius_range, integrale di linea sulla circonferenza con origine pari a centro
 */
vector<int> linear_integral_vector_multi(image* img, Point centro, vector<int> radius_range, string col){
	vector<int> line_integral;
	for( int i = 0; i < radius_range.size(); i++ ){					// ignoro l'arco superiore ( > 45° fino a < 135° ) poiché non viene considerato all'interno del calcolo di integrale lineare
		//int p1_y = radius_range[i] * sin(2*M_PI/8) + centro.y;		// quindi prendo le coordinate y estreme ( a 45° e a 135° ) che mi consentono di calcolare l'integrale anche se la circonferenza
	//	int p2_y = radius_range[i] * sin((2*M_PI*3)/8) + centro.y;	// non include l'arco superiore nell'immagine
		if( centro.x - radius_range[i] < 0 || centro.x + radius_range[i] > img->cols || centro.y + radius_range[i] > img->rows || (centro.y - radius_range[i]) < 0) // oppure  p1_y < 0 || p2_y < 0 
		{
			line_integral.push_back(0);
		}
		else{
			int tmp = contour_sum_multi(img, centro, radius_range[i], col);
			line_integral.push_back(tmp);
		}
	}
	return line_integral;
}

/**
 * @brief applicazione del multi-spectral integro-differential operator di Daugman (vedi Krupicka e Daugman) in modalità multi-spettro discretizzato per ottenere il contorno dell'iride (limbus)
 * @param img struct contenente matrici dei 3 canali (BGR), numero di colonne, numero di righe (width e height)
 * @param r_min raggio minimo preso in considerazione. Di norma equivale a height/4
 * @param r_max raggio massimo preso in considerazione. Di norma equivale a height/2
 * @return results struct contenente raggio, punto centrale (x,y) e valore massimo. Il valore massimo equivale al valore massimo che l'operatore integro-differenziale 
 * 		ottiene in base ai vari raggi e punti centrali presi in considerazione
 */
results* apply_daugman_operator_multi(image* img, int r_min, int r_max){
	//cout << "inizia" << endl;
	double sigma = SIGMA;
	int rows = img->rows;
	int cols = img->cols;
	results* res = (results*) calloc(1, sizeof(results));
	/*r_max = r_max + (DELTA_R/2);		// così da usare tutti i raggi, altrimenti andrei a salter gli ultimi DELTA_R/2 raggi
	r_min = r_min - (DELTA_R/2) -1;		// così da usare tutti i raggi, altrimenti andrei a salter i primi DELTA_R/2 -1 raggi*/
	vector<int> radius_range(r_max-r_min+1);
	std::iota(begin(radius_range), end(radius_range), r_min);	// inserisco in radius_range r_max-r_min valori, partendo da r_min e incrementando di 1 ogni volta che inserisco	
	double max = 0;
	Mat convolved_blue;
	Mat convolved_green;
	Mat convolved_red;
	Mat convolved(2, radius_range.size(), CV_64F);
	for( int y = 0; y < rows; y++ ){
		for( int x = 0; x < cols; x++ ){
			Point centro(x,y);
			vector<int> line_int_blue = linear_integral_vector_multi(img, centro, radius_range, "blue");	// applico, per ogni raggio appartenente a radius_range, l'integrale lineare al cerchio con centro x,y
			vector<int> line_int_green = linear_integral_vector_multi(img, centro, radius_range, "green");	// applico, per ogni raggio appartenente a radius_range, l'integrale lineare al cerchio con centro x,y
			vector<int> line_int_red = linear_integral_vector_multi(img, centro, radius_range, "red");	// applico, per ogni raggio appartenente a radius_range, l'integrale lineare al cerchio con centro x,y
			if( find(line_int_blue.begin(), line_int_blue.end(), 0) != line_int_blue.end() ){
				int addr = getIndexOfZeros(line_int_blue);	// ottengo l'indirizzo in line_int che contiene il primo zero. Serve per ottenere una slice utilizzabile di line_int, perché è possibile che
														// per alcuni raggi il cerchio con centro x,y esca fuori, ma questo non sempre vale per tutti i raggi con tale centro
				if( addr == 0 || addr < DELTA_R +1 ) continue; 	// se l'indirizzo è minore di DELTA_R+1, significa che ci sono meno di DELTA_R+1 valori utilizzabili in line_int, e quindi
															  	// il kernel non può scorrere per n-k e/o n-k-1
				vector<int> new_line_int_blue = slice(line_int_blue, 0, addr);	// ottengo la slice utilizzabile
				vector<int> new_line_int_green = slice(line_int_green, 0, addr);	// ottengo la slice utilizzabile
				vector<int> new_line_int_red = slice(line_int_red, 0, addr);	// ottengo la slice utilizzabile
				convolved_blue = convolution(sigma, &new_line_int_blue);		// applico la convoluzione
				convolved_green = convolution(sigma, &new_line_int_green);		// applico la convoluzione
				convolved_red = convolution(sigma, &new_line_int_red);		// applico la convoluzione
				double val = 0;
				for(int convCols = 0; convCols < addr-((DELTA_R/2)*2)-1; convCols++){
					convolved.at<double>(0,convCols) = convolved_blue.at<double>(0,convCols) + convolved_green.at<double>(0,convCols) + convolved_red.at<double>(0,convCols);
					convolved.at<double>(1,convCols) = convolved_blue.at<double>(1,convCols) + convolved_green.at<double>(1,convCols) + convolved_red.at<double>(1,convCols);					
				}
				// in convolved ho n valori quanti sono i raggi, ma gli ultimi (delta_r/2)*2 non sono utilizzabili (all'indice 0 corrisponde il primo elemento utile, a (delta_r/2)*2-1 l'ultimo utile)
				// perché non prendo i raggi troppo a sinistra e troppo a destra (dove non posso applicare la convoluzione con kernel
				for( int ind = 0; ind < convolved.cols-((DELTA_R/2)*2)-1; ind++ ){
					val = abs(convolved.at<double>(0, ind) - convolved.at<double>(1, ind))/DELTA_R;	// per ogni valore in convoluzione, faccio abs(conv(n-k) - conv(n-k-1))/delta_r
					if( val > max ){		// sostituisco il valore ( e lo valuto come possibile candidato limbus ) solo se è quello con valore maggiore (vedi operatore di Daugman)
						max = val;
						res->radius = radius_range[ind+(DELTA_R/2)+1];
						res->center = centro;
						res->value = val;
					}
				}
			}
			else{	// se non ci sono 0 all'interno di line_int significa che tutti i valori sono utilizzabili, e quindi per ogni cerchio con raggio in radius_range e centro x,y non c'è alcun problema 
				convolved_blue = convolution(sigma, &line_int_blue);		// applico la convoluzione
				convolved_green = convolution(sigma, &line_int_green);		// applico la convoluzione
				convolved_red = convolution(sigma, &line_int_red);		// applico la convoluzione
				double val = 0;
				for(int convCols = 0; convCols < convolved_blue.cols-((DELTA_R/2)*2)-1; convCols++){
					convolved.at<double>(0,convCols) = convolved_blue.at<double>(0,convCols) + convolved_green.at<double>(0,convCols) + convolved_red.at<double>(0,convCols);
					convolved.at<double>(1,convCols) = convolved_blue.at<double>(1,convCols) + convolved_green.at<double>(1,convCols) + convolved_red.at<double>(1,convCols);					
				}
				// in convolved ho n valori quanti sono i raggi, ma gli ultimi (delta_r/2)*2 non sono utilizzabili (all'indice 0 corrisponde il primo elemento utile, a (delta_r/2)*2-1 l'ultimo utile)
				// perché non prendo i raggi troppo a sinistra e troppo a destra (dove non posso applicare la convoluzione con kernel
				for( int ind = 0; ind < convolved.cols-((DELTA_R/2)*2)-1; ind++ ){
				/*	cout << "####" << endl;
					cout << convolved.at<double>(0,ind) << endl;
					cout << convolved.at<double>(1,ind) << endl;
					cout << "####" << endl;*/
					val = abs(convolved.at<double>(0, ind) - convolved.at<double>(1, ind))/DELTA_R;	// per ogni valore in convoluzione, faccio abs(conv(n-k) - conv(n-k-1))/delta_r
					if( val > max ){		// sostituisco il valore ( e lo valuto come possibile candidato limbus ) solo se è quello con valore maggiore (vedi operatore di Daugman)
						max = val;
						res->radius = radius_range[ind+(DELTA_R/2)+1];
						res->center = centro;
						res->value = val;
					}
				}
			}
		}
	}
	return res;
}