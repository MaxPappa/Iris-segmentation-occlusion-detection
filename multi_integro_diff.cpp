#include "multi_integro_diff.h"

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
 * @return la quantità di pixel presenti sulla circonferenza del cerchio con raggio r
 */
int getAmountCirclePixels(int r){
	return 4*round(2*((1/(sqrt(2)) * r)));
}


/**
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
 *	@param img puntatore alla struct image (contenente puntatori alle matrici dei tre spettri di colore dell'immagine)
 *	@param centro punto centrale (x,y) della circonferenza presa in considerazione
 *	@param radius_range vettore contenente i raggi presi in considerazione ( da r_min a r_max )
 *	@return line_integral vettore contenente un integrale di linea per ogni raggio presente in radius_range, integrale di linea sulla circonferenza con origine pari a centro
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
 * @see contour_sum la differenza è che per la pupilla calcolo l'integrale discretizzato ( somma dei pixel sul perimetro della circonferenza ) per l'intera circonferenza, perché
 *		si suppone non ci siano occlusioni dovute a ciglia e/o palpebra superiore
 * @param img_red matrice del canale RED dell'immagine di input
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
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
 * @see pixel_value la differenza è che per la pupilla, avendo un singolo canale (il rosso) non faccio nessuna somma pesata
 * @param img_red matrice del canale RED dell'immagine di input
 * @param angle angolo attualmente preso in considerazione, viene utilizzato nel calcolo delle coordinate del punto (x,y) all'angolo angle della circonferenza presa in considerazione
 * @param centro punto centrale (x,y) della circonferenza presa in considerazione
 * @param r raggio della circonferenza presa in considerazione
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


pair<int,int> obtain_w_h_miche(int cols, int rows){
	//if(cols/256 <= 0) return pair<int,int>(0,0);
	int k = cols;
	//int h = cols/2;
	//if(h <= 128) return pair<int,int>(cols/h, rows/h);
	int count = 1;
	bool flag = false; bool next = true;
	while((k > 42 || flag) && next){
		//cout << k << endl;
		for(int i = 2; i <= k; i++){
			if(k%i == 0){
			//	cout << "k = " << k << ", i = " << i << endl;
				if(k/i > 42)
					k = k/i;
				else next = false;
				break;
			}
			else if(i == k) flag = true;
		}
	}
	pair<int,int> coppia(k,k);
	/*while(k%2 == 0 && k/2 >= 128){
		k = k/2;
		count++;
	}*/
//	pair<int,int> coppia(cols/(count*2), rows/(count*2));
	/*int k = h;

	if(k <= 128) return;
	while(cols%k != 0 || rows%k != 0){
		k--;
	}
	pair<int,int> coppia(cols/k, rows/k);*/
	return coppia;
}


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
 * @brief creo le directory presenti in inp_path all'interno di out_path e ritorno i rispettivi path (ritorno out_vec che contiene tali path). N.B.: valido per Utiris
 * @param inp_path path alla directory di input (in cui si trova il DB, in questo caso Utiris)
 * @param out_path path alla directory di output in cui andranno a crearsi tante dirs quante sono in inp_path
 * @return vector<string> out_vec contenente i path alle singole directory create
 */
/*vector<string> create_dirs(char* inp_path, char* out_path){ // per Utiris_DB
	vector<string> out_vec;
	DIR *inp_dir; DIR *out_dir; DIR *eye_dir;
	struct dirent *ent;
	struct dirent *eye_file;
	int check;
	char filename[256]; char* extension; char file_n[256]; char eye_path[256];
	if ((inp_dir = opendir(inp_path)) != NULL && (out_dir = opendir(out_path)) != NULL) {
		while ((ent = readdir (inp_dir)) != NULL) {
			if(ent->d_name[0] == '.') continue;
			extension = ent->d_name;
			strncpy(filename, out_path, sizeof(filename)); 			// non faccio controlli dkw
			strncpy(file_n, inp_path, sizeof(file_n));
			strncat(filename, extension, (sizeof(filename) - strlen(filename)));		// non faccio controlli dkw
			strncat(file_n, extension, (sizeof(file_n) - strlen(file_n)));

			check = mkdir(filename, 0777);
			if(check){cout << "errore nella creazione della cartella " << ent->d_name << endl; break;}
			strncat(file_n, "/", (sizeof(file_n) - strlen(file_n)));
			eye_dir = opendir(file_n);
			while ((eye_file = readdir (eye_dir)) != NULL){
				if(eye_file->d_name[strlen(eye_file->d_name)-1] == 'G' || eye_file->d_name[strlen(eye_file->d_name)-1] == 'g'){		// controllo solo se è JPG ma per ora mi basta solo la G
					strncpy(eye_path, file_n, sizeof(eye_path));
					strncat(eye_path, eye_file->d_name, (sizeof(eye_path) - strlen(eye_path)));
					string s(eye_path);
					out_vec.push_back(s);
					//cout<<eye_path<<endl;
				}
			}
			closedir(eye_dir);
		}
		closedir(inp_dir);
		closedir(out_dir); 
	}
	//print(out_vec);
	return out_vec;
}
*/

/**
 * @brief applicazione del multi-spectral integro-differential operator di Daugman (vedi Krupicka e Daugman) discretizzato per ottenere il contorno dell'iride (limbus)
 * @param img struct contenente matrici dei 3 canali (BGR), numero di colonne, numero di righe (width e height)
 * @param r_min raggio minimo preso in considerazione. Di norma equivale a height/4
 * @param r_max raggio massimo preso in considerazione. Di norma equivale a height/2
 * @return results struct contenente raggio, punto centrale (x,y) e valore massimo. Il valore massimo equivale al valore massimo che l'operatore integro-differenziale 
 * 		ottiene in base ai vari raggi e punti centrali presi in considerazione
 */
results* apply_daugman_operator(image* img, int r_min, int r_max){
	double sigma = 0.5;
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
	double sigma = 0.5;
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
				if( addr == 0 || addr < DELTA_R +1 ) continue;	// se l'indirizzo è minore di DELTA_R+1, significa che ci sono meno di DELTA_R+1 valori utilizzabili in line_int, e quindi
															  	// il kernel non può scorrere per n-k e/o n-k-1
				vector<int> new_line_int = slice(line_int, 0, addr);	// ottengo la slice utilizzabile
				convolved = pup_convolution(img_red, sigma, &new_line_int, radius_range, centro);		// applico la convoluzione per pupilla
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
				convolved = pup_convolution(img_red, sigma, &line_int, radius_range, centro);	// applico la convoluzione per pupilla
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
	vector<double> kernel = getGaussianKernel(DELTA_R, sigma);	// creo il 1-D gaussian Kernel. La funzione ritorna un DELTA_R X 1 Mat, ma facendo cast a vector, diventa vetctor 1xDELTA_R
	int li_size = line_int->size();
	Mat target(2, li_size, CV_64F);
	int l = 0;
	double value; int divider;
	for( int pos = DELTA_R/2+1; pos < li_size-DELTA_R/2; pos++ ){// non prendo come centro i raggi che farebbero andare troppo a sinistra o troppo a destra i valori del kernel (i lati escono dal range)
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


Mat binaryMask(Mat* src_image, int iris_r, Point iris_c, int pupil_r, Point pupil_c){
	Mat mask = Mat::zeros(Size(src_image->cols, src_image->rows), CV_8UC1);
	circle(mask, iris_c, iris_r, Scalar(255), -1); // thickness = -1 -> filled circle
	circle(mask, pupil_c, pupil_r, Scalar(0), -1);
	return mask;
}


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


results* apply_daugman_operator_multi(image* img, int r_min, int r_max){
	//cout << "inizia" << endl;
	double sigma = 0.5;
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

// fine multi convoluzione


/*
 * main per il database MICHE
 */
/*int main(int argc, char *argv[]){
	vector<string> vec_paths = create_dirs("/home/max/Desktop/Tirocinio_Iris/Miche_1/", "/home/max/Desktop/Test/Miche_Prove/");
	auto start = std::chrono::system_clock::now();


	//#pragma omp parallel for
	for( int i = 0; i < vec_paths.size(); i++){
	//	Mat img_in = imread("/home/max/Desktop/iPhone5/004_IP5_IN_F_LI_01_4.jpg", 1);
		vector<string> splitted_1 = split(vec_paths[i], '/');
		string destinazione_1 = splitted_1[splitted_1.size()-2];
		string name_1 = splitted_1[splitted_1.size()-1];
		Mat img_in = imread(vec_paths[i], 1);
		cout << "start with " << vec_paths[i] << endl;
		Mat inp_img;
		Mat mask_prev;
		search_reflection(&img_in, &mask_prev, 3, -20);
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione_1+"/"+name_1+"_reflecMask.png", mask_prev);
		inpaint_reflection(&img_in, &mask_prev, &inp_img, 1);
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione_1+"/"+name_1+"_inpainted.png", inp_img);


		Mat grayscaled;
		cvtColor(img_in, grayscaled, COLOR_BGR2GRAY);
		//Mat inp_can;
		CascadeClassifier casc("/home/max/Desktop/Tirocinio_Iris/haarcascade_eye/haarcascade_eye.xml");
		vector<Rect> detections;
		casc.detectMultiScale(inp_img, detections, 1.05, 5, 0, Size(250,250));
		if(detections.size() == 0) {cout << 0; continue;}
		Mat roiMat; int ind = 0;
		for(int j = 0; j < detections.size(); j++){
			if(detections[ind].width < detections[j].width)
				ind = j;
		}
		Rect rec = detections[ind];
		bool flag_b = false;
		int def_width = rec.width;
		int chWid = checkWidth(rec.width);
		while((chWid > 256 || chWid < 128)){
			rec.width = rec.width+1;
			rec.height = rec.height+1;
			chWid = checkWidth(rec.width);
			if(def_width < rec.width) {flag_b = true; break;}
		}
		if(flag_b){
			rec.width = def_width;
			rec.height = def_width;
			chWid = checkWidth(rec.width);
			while(chWid > 256 || chWid < 128){
				rec.width = rec.width-1;
				rec.height = rec.height-1;
				chWid = checkWidth(rec.width);
			}
		}
		roiMat = img_in(rec);
		cout << vec_paths[i] << endl;

		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione_1+"/"+name_1+"_1.jpg", roiMat);

//	}
		Mat img_color = roiMat.clone();

		Mat imgToNorm = img_color.clone();
		pair<int,int> w_h = obtain_w_h_miche(img_color.cols, img_color.rows);
		int width = w_h.first;
		int height = w_h.second;
		cout << vec_paths[i] << ", " << width << "x" << height << endl;
		if(width == 0 || height == 0) continue;
		Mat resized;
		resize(img_color, resized, Size(width, height));

		Mat mask;
		search_reflection(&resized, &mask, 3, -20);
		Mat inpainted;
	  	inpaint_reflection(&resized, &mask, &inpainted, 1);

		Mat bgr[3];
		split(inpainted, bgr);



		int r_min = height/8; // /6 per utiris, /8 per miche
		int r_max = height/3; // /2 per utiris, /3 per miche
		printf("r_min = %d, r_max = %d\n", r_min, r_max);

		image* img = (image*) calloc(1, sizeof(image));

		Mat rid_blue, rid_green, rid_red;
		resize(bgr[0], rid_blue, Size(width, height));
		img->blue = &rid_blue;
		resize(bgr[1], rid_green, Size(width, height));
		img->green = &rid_green;
		resize(bgr[2], rid_red, Size(width, height));
		img->red = &rid_red;
		img->cols = rid_blue.cols;
		img->rows = rid_blue.rows;
		
		results* res =  apply_daugman_operator_multi( img, r_min, r_max);

		int new_radius = res->radius*(img_color.cols/width);
		Point center(res->center.x*(img_color.cols/width), res->center.y*(img_color.rows/height));
		circle(img_in, Point(center.x+rec.x,center.y+rec.y), new_radius, Scalar(0,0,255), 2);
		printf("value = %f, radius = %d, center(x,y) = %d,%d \n", res->value, res->radius, res->center.x, res->center.y);
		
		int r_pup_min = res->radius / 8;
		if( r_pup_min == 0 ) r_pup_min = 1;
		int r_pup_max = res->radius * 0.85;	// 0.85 per utiris, 0.7 per miche
		cout << "r_pup_min = " << r_pup_min << ", r_pup_max = "  << r_pup_max << endl;
		int x_roi = res->center.x - res->radius;
		if( x_roi < 0 ) x_roi = 0;
		int y_roi = res->center.y - res->radius;
		if( y_roi < 0 ) y_roi = 0;
		CvRect roi = cvRect(x_roi, y_roi, res->radius*2, res->radius*2);
		Mat roiImg;
		roiImg = inpainted(roi);
		
		Mat bgr_pup[3];
		split(roiImg, bgr_pup);
		Mat img_red = bgr_pup[2];
		
		results* res_pup = pupil_daugman_operator(&img_red, r_pup_min, r_pup_max);
		
		int x_pup = (x_roi + res_pup->center.x) * (img_color.cols/width);
		int y_pup = (y_roi + res_pup->center.y) * (img_color.cols/width);
		Point center_pup(x_pup, y_pup);
		int radius_pup = res_pup->radius * (img_color.cols/width);
		circle(img_in, Point(center_pup.x+rec.x,center_pup.y+rec.y), radius_pup, Scalar(0,0,255), 2);
		printf("value_pup = %f, radius_pup = %d, center_pup(x,y) = %d,%d \n", res_pup->value, res_pup->radius, res_pup->center.x, res_pup->center.y);
		vector<string> splitted = split(vec_paths[i], '/');
		string destinazione = splitted[splitted.size()-2];
		string name = splitted[splitted.size()-1];
		cout << destinazione << endl;
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione+"/"+name, img_in);
		
		Mat finalMask = binaryMask(&inp_img, new_radius, Point(center.x+rec.x,center.y+rec.y), radius_pup, Point(center_pup.x+rec.x,center_pup.y+rec.y));

		Mat bin_mask = binaryMask(&img_color, new_radius, center, radius_pup, center_pup);
		/*Mat out_mask;
		imgToNorm.copyTo(out_mask, bin_mask);*/
		
		//imwrite("/home/max/Desktop/Tirocinio_Iris/ProvaConMask/"+destinazione+"mask.JPG", out_mask);
	/*	map<string, Point> mappa;
		Mat out_img = normalizza(&imgToNorm, new_radius, center.x, center.y, radius_pup, center_pup.x, center_pup.y, &mappa);
		/*for(map<string,Point>::iterator it=mappa.begin(); it!=mappa.end(); ++it){
			//cout << it->first << " corrisponde al pixel " << it->second.x << "," << it->second.y << endl;
			imgToNorm.at<Vec3b>(it->second.y, it->second.x) = {255,255,255};
		}*/



/*		string destinazione2 = vec_paths[i].substr(vec_paths[i].length()-20, vec_paths[i].length()-1);
		string folder = destinazione2.substr(1, 3);
		cout << destinazione2 << endl;
	//	cout << folder << endl;
		cout << vec_paths[i] << endl;
		Mat norm_bgr[3];
		split(out_img, norm_bgr);
		Mat upperEyelidMask = upperEyelidDetection(&norm_bgr[2], destinazione+"/"+name);
		cout << "upper" << endl;
		//imwrite("/home/max/Desktop/raydrawUpper.png", norm_bgr[2]);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Miche_Individuation/"+destinazione+"/"+name+"_upperEyelidMask.png", upperEyelidMask);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Sample/"+destinazione2+"_redSpectrum.png", norm_bgr[2]);
		Mat lowerEyelidMask = lowerEyelidDetection(&norm_bgr[2]);
		cout << "lower" << endl;
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Miche_Individuation/"+destinazione+"/"+name+"_lowerEyelidMask.png", lowerEyelidMask);
		Mat ultimateMask = Mat(lowerEyelidMask.rows, lowerEyelidMask.cols, CV_8UC1, Scalar(255));
		Mat reflectionMask = threshReflectionDetection(&norm_bgr[0], 5, -15);
		cout << "reflection" << endl;
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Miche_Individuation/"+destinazione+"/"+name+"_reflMask.png", reflectionMask);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Sample/"+destinazione2+"_blueSpectrum.png", norm_bgr[0]);
		for(int y = 0; y < ultimateMask.rows; y++){
			for(int x = 0; x < ultimateMask.cols; x++){
				if(lowerEyelidMask.at<uchar>(y,x) == 0 || upperEyelidMask.at<uchar>(y,x) == 0 || reflectionMask.at<uchar>(y,x) != 0){
					ultimateMask.at<uchar>(y,x) = 0;
					Point pt = mappa[to_string(x)+","+to_string(y)];
					bin_mask.at<uchar>(pt.y, pt.x) = 0;
					finalMask.at<uchar>(rec.y+pt.y, rec.x+pt.x) = 0;
					//cout << pt << endl;
				}
			}
		}
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Miche_Individuation/"+destinazione+"/"+name+"_ultimateMask.png", ultimateMask);
		Mat maskResized;
		resize(ultimateMask, maskResized, Size(600, 100));
		Mat normResized;
		resize(out_img, normResized, Size(600,100));
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione+"/"+name+"_finalMask.png", finalMask);
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione+"/"+name+"_norm.png", normResized);
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione+"/"+name+"_maskRicomposta.png", bin_mask);
		imwrite("/home/max/Desktop/Test/Miche_Prove/"+destinazione+"/"+name+"_ultimateMask.png", maskResized);

		free(res_pup);
		free(res);
		free(img);
	}
	auto end = chrono::system_clock::now();
	time_t end_time = chrono::system_clock::to_time_t(end);
	time_t start_time = chrono::system_clock::to_time_t(start);
	cout << "started computation at " << ctime(&start_time);
	cout << "finished computation at " << ctime(&end_time);
}

/*
 * main per il database Utiris.
 * ? potrei chiamare questo metodo "runUtiris" e l'altro "runMiche" e chiamare ambedue dal main generico.
 */
/*int main(int argc, char *argv[]){
	vector<string> vec_paths = create_dirs("/home/max/Desktop/Tirocinio_Iris/Utiris_DB/RGB_Chiari/", "/home/max/Desktop/Test/UltimiTestUtiris/");
	
	auto start = std::chrono::system_clock::now();
	
	//#pragma omp parallel for
	for( int i = 0; i < vec_paths.size(); i++){
		
		Mat img_color = imread(vec_paths[i], 1);

		Mat imgToNorm = img_color.clone();
		pair<int,int> w_h = obtain_w_h(img_color.cols, img_color.rows);
		int width = w_h.first;
		int height = w_h.second;
		Mat resized;
		resize(img_color, resized, Size(width, height));

		Mat mask;
		search_reflection(&resized, &mask, 3, -20);
		Mat inpainted;
	  	inpaint_reflection(&resized, &mask, &inpainted, 1);

		Mat bgr[3];
		split(inpainted, bgr);



		int r_min = height/6;
		int r_max = height/2;
		printf("r_min = %d, r_max = %d\n", r_min, r_max);

		image* img = (image*) calloc(1, sizeof(image));

		Mat rid_blue, rid_green, rid_red;
		resize(bgr[0], rid_blue, Size(width, height));
		img->blue = &rid_blue;
		resize(bgr[1], rid_green, Size(width, height));
		img->green = &rid_green;
		resize(bgr[2], rid_red, Size(width, height));
		img->red = &rid_red;
		img->cols = rid_blue.cols;
		img->rows = rid_blue.rows;
		
		results* res =  apply_daugman_operator( img, r_min, r_max);

		int new_radius = res->radius*(img_color.cols/width);
		Point center(res->center.x*(img_color.cols/width), res->center.y*(img_color.rows/height));
		circle(img_color, center, new_radius, Scalar(0,0,255), 3);
		printf("value = %f, radius = %d, center(x,y) = %d,%d \n", res->value, res->radius, res->center.x, res->center.y);
		
		int r_pup_min = res->radius / 6;
		if( r_pup_min == 0 ) r_pup_min = 1;
		int r_pup_max = res->radius * 0.85;
		cout << "r_pup_min = " << r_pup_min << ", r_pup_max = "  << r_pup_max << endl;
		int x_roi = res->center.x - res->radius;
		if( x_roi < 0 ) x_roi = 0;
		int y_roi = res->center.y - res->radius;
		if( y_roi < 0 ) y_roi = 0;
		CvRect roi = cvRect(x_roi, y_roi, res->radius*2, res->radius*2);
		Mat roiImg;
		roiImg = inpainted(roi);
		
		Mat bgr_pup[3];
		split(roiImg, bgr_pup);
		Mat img_red = bgr_pup[2];
		
		results* res_pup = pupil_daugman_operator(&img_red, r_pup_min, r_pup_max);
		
		int x_pup = (x_roi + res_pup->center.x) * (img_color.cols/width);
		int y_pup = (y_roi + res_pup->center.y) * (img_color.cols/width);
		Point center_pup(x_pup, y_pup);
		int radius_pup = res_pup->radius * (img_color.cols/width);
		circle(img_color, center_pup, radius_pup, Scalar(0,0,255), 3);
		printf("value_pup = %f, radius_pup = %d, center_pup(x,y) = %d,%d \n", res_pup->value, res_pup->radius, res_pup->center.x, res_pup->center.y);
		
		string destinazione = vec_paths[i].substr(vec_paths[i].length()-20, vec_paths[i].length()-1);
		string name = destinazione.substr(0, destinazione.length()-4);
		cout << destinazione << endl;
		imwrite("/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione, img_color);
		
		Mat bin_mask = binaryMask(&img_color, new_radius, center, radius_pup, center_pup);
		Mat out_mask;
		imgToNorm.copyTo(out_mask, bin_mask);
		
		//imwrite("/home/max/Desktop/Tirocinio_Iris/ProvaConMask/"+destinazione+"mask.JPG", out_mask);
		map<string, Point> mappa;
		Mat out_img = normalizza(&imgToNorm, new_radius, center.x, center.y, radius_pup, center_pup.x, center_pup.y, &mappa);
		
		for(map<string,Point>::iterator it=mappa.begin(); it!=mappa.end(); ++it){
			//cout << it->first << " corrisponde al pixel " << it->second.x << "," << it->second.y << endl;
			imgToNorm.at<Vec3b>(it->second.y, it->second.x) = {255,255,255};
		}

		string destinazione2 = vec_paths[i].substr(vec_paths[i].length()-20, vec_paths[i].length()-1);
		string folder = destinazione2.substr(1, 3);
		cout << destinazione2 << endl;
		cout << folder << endl;
		cout << vec_paths[i] << endl;
		Mat norm_bgr[3];
		Mat normResized_new;
		resize(out_img, normResized_new, Size(600,100));
		split(out_img, norm_bgr);
		imwrite("/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione2+"normalizzataNormale.png", normResized_new);
			Mat revUpper = Mat::zeros(normResized_new.rows, normResized_new.cols, CV_8UC3);	// ottengo l'iride normalizzata con al centro l'upper eyelid
	for(int ind = 0; ind < normResized_new.cols/2; ind++){
		for(int j = 0; j < normResized_new.rows; j++){
			revUpper.at<Vec3b>(j,ind) = normResized_new.at<Vec3b>(j,ind+normResized_new.cols/2);
		}
	}
	for(int ind = normResized_new.cols/2; ind < normResized_new.cols; ind++){
		for(int j = 0; j < normResized_new.rows; j++){
			revUpper.at<Vec3b>(j,ind) = normResized_new.at<Vec3b>(j,ind-normResized_new.cols/2);
		}
	}
	imwrite("/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione2+"normalizzataReversed.png", revUpper);
		Mat upperEyelidMask = upperEyelidDetection(&norm_bgr[2], "/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione2);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Sample/"+destinazione2+"_upperEyelidMask.png", upperEyelidMask);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Sample/"+destinazione2+"_redSpectrum.png", norm_bgr[2]);
		Mat lowerEyelidMask = lowerEyelidDetection(&norm_bgr[2]);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Sample/"+destinazione2+"_lowerEyelidMask.png", lowerEyelidMask);
		Mat ultimateMask = Mat(lowerEyelidMask.rows, lowerEyelidMask.cols, CV_8UC1, Scalar(255));
		Mat reflectionMask = threshReflectionDetection(&norm_bgr[0], 5, -15);
	//	imwrite("/home/max/Desktop/Tirocinio_Iris/ProveUltime/"+destinazione2+"_reflMask.png", reflectionMask);
		//imwrite("/home/max/Desktop/Tirocinio_Iris/Sample/"+destinazione2+"_blueSpectrum.png", norm_bgr[0]);
		for(int y = 0; y < ultimateMask.rows; y++){
			for(int x = 0; x < ultimateMask.cols; x++){
				if(lowerEyelidMask.at<uchar>(y,x) == 0 || upperEyelidMask.at<uchar>(y,x) == 0 || reflectionMask.at<uchar>(y,x) != 0){
					ultimateMask.at<uchar>(y,x) = 0;
					Point pt = mappa[to_string(x)+","+to_string(y)];
					bin_mask.at<uchar>(pt.y, pt.x) = 0;
				}
			}
		}
		Mat maskResized;
		resize(ultimateMask, maskResized, Size(600, 100));
		Mat normResized;
		resize(out_img, normResized, Size(600,100));
		imwrite("/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione2+"_norm.png", normResized);
		imwrite("/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione2+"_maskRicomposta.png", bin_mask);
		imwrite("/home/max/Desktop/Test/UltimiTestUtiris/"+destinazione2+"_ultimateMask.png", maskResized);

		free(res_pup);
		free(res);
		free(img);
	}
	auto end = chrono::system_clock::now();
	time_t end_time = chrono::system_clock::to_time_t(end);
	time_t start_time = chrono::system_clock::to_time_t(start);
	cout << "started computation at " << ctime(&start_time);
	cout << "finished computation at " << ctime(&end_time);
}



/*int main(int argc, char *argv[]){
	vector<string> vec_paths = create_dirs("/home/max/Desktop/Test/Test_Normalizzazione/", "/home/max/Desktop/Tirocinio_Iris/ProvaNorm/");
	
	auto start = std::chrono::system_clock::now();

	//#pragma omp parallel for
	for( int i = 0; i < vec_paths.size(); i++){
		string destinazione = vec_paths[i].substr(vec_paths[i].length()-20, 11);
		string folder = destinazione.substr(4, 3);
		cout << destinazione << endl;
		cout << folder << endl;
		cout << vec_paths[i] << endl;
		Mat img_color = imread(vec_paths[i], 1);	
		Mat norm_bgr[3];
		split(img_color, norm_bgr);
		Mat notGauss = norm_bgr[2].clone();
		GaussianBlur( norm_bgr[2], norm_bgr[2], Size(41,41), 0, 0, BORDER_DEFAULT );
		Mat newbgr = norm_bgr[2].clone();
		Mat upperEyelidMask = upperEyelidDetection(&norm_bgr[2], &newbgr, &notGauss);
		imwrite("/home/max/Desktop/raydrawUpper.png", norm_bgr[2]);
		imwrite("/home/max/Desktop/normbgr.png", newbgr);
		imwrite("/home/max/Desktop/Tirocinio_Iris/ProvaNorm/"+folder+"/"+destinazione+"_upperEyelidMask.png", upperEyelidMask);
		imwrite("/home/max/Desktop/Tirocinio_Iris/ProvaNorm/"+folder+"/"+destinazione+"_norm.png", img_color);
		imwrite("/home/max/Desktop/Tirocinio_Iris/ProvaNorm/"+folder+"/"+destinazione+"_upper.png", newbgr);
	}
	auto end = chrono::system_clock::now();
	time_t end_time = chrono::system_clock::to_time_t(end);
	time_t start_time = chrono::system_clock::to_time_t(start);
	cout << "started computation at " << ctime(&start_time);
	cout << "finished computation at " << ctime(&end_time);
}*/



/*int main(int argc, char *argv[]){
	vector<string> names = getFileNames("/home/max/Desktop/Univ/ground_truth/Miche");
	float f1_mio = 0;
	float f1_krup = 0;
	float tot_acc_krup = 0;
	float tot_acc_mio = 0;
	float tot_spec_mio = 0;
	float tot_spec_krup = 0;
	//vector<string> names = getFileNames("/home/max/Desktop/Univ/Tesi/micheip5");
	for(int i = 0; i < names.size(); i++){
		cout << names[i] << endl;
		//Mat GTmask = imread("/home/max/Desktop/Univ/ground_truth/Utiris/"+names[i], 0);
		//Mat GTmask = imread("/home/max/Desktop/Univ/Tesi/micheip5/"+names[i], 0);
		Mat GTmask = imread("/home/max/Desktop/Univ/ground_truth/Miche/"+names[i], 0);
		int len = names[i].length();
		int index = len - 9;
		string maskName = names[i].replace(index, 9, "");
		cout << maskName << endl;
		//string mio = maskName;
	//	mio+=".jpg_finalMask.png";
		//cout << mio << endl;
		Mat mask_mio = imread("/home/max/Desktop/Univ/ground_truth/MioAlgo/"+maskName+".jpg_finalMask.png", 0);
	//	Mat mask_mio = imread("/home/max/Desktop/Univ/ground_truth/Utiris_MioAlgo/"+maskName+".JPG_maskRicomposta.png", 0);
	//	Mat mask_mio = imread("/home/max/Desktop/Univ/Tesi/MioAlgo/"+maskName+"_finalMask.png", 0);
		Mat mask_krup = imread("/home/max/Desktop/Univ/ground_truth/Krupicka/"+maskName+".defects.png", 0);
	//	Mat mask_krup = imread("/home/max/Desktop/Univ/ground_truth/Utiris_Krupicka/"+maskName+".defects.png", 0);
	//	Mat mask_krup = imread("/home/max/Desktop/Univ/Tesi/Krupicka/"+maskName+"_defects.png", 0);
		float score_mio = F1_Score(&GTmask, &mask_mio);
		f1_mio+=score_mio;
		cout << "mio F1-Score = " << score_mio << endl;
		float score_krup = F1_Score(&GTmask, &mask_krup);
		f1_krup+=score_krup;
		cout << "krupicka F1-Score = " << score_krup << endl;

		float acc_mio = accuracy(&GTmask, &mask_mio);
		tot_acc_mio+=acc_mio;
		cout << "mio accuracy = " << acc_mio << endl;

		float acc_krup = accuracy(&GTmask, &mask_krup);
		tot_acc_krup+=acc_krup;
		cout << "krup accuracy = " << acc_krup << endl;

		float spec_mio = specificity(&GTmask, &mask_mio);
		tot_spec_mio+=spec_mio;
		cout << "mio specificity = " << spec_mio << endl;

		float spec_krup = specificity(&GTmask, &mask_krup);
		tot_spec_krup+=spec_krup;
		cout << "krup specificity = " << spec_krup << endl;

		cout << "##############" << endl;
	}
	cout << "media F1 mio = " << f1_mio/names.size() << endl;
	cout << "media F1 krup = " << f1_krup/names.size() << endl;
	cout << "media accuracy mio = " << tot_acc_mio/names.size() << endl;
	cout << "media accuracy krup = " << tot_acc_krup/names.size() << endl;
	cout << "media specificity mio = " << tot_spec_mio/names.size() << endl;
	cout << "media specificity krup = " << tot_spec_krup/names.size() << endl;

	/*Mat GTmask = imread("/home/max/Desktop/Univ/ground_truth/Miche/068_GS4_OU_R_RI_01_4_mask.png", 0);
	//Mat mask_mio = imread("/home/max/Desktop/Tirocinio_Iris/ProveUltime/069/IMG_069_R_1.JPG_maskRicomposta.png", 0);
	Mat mask_mio = imread("/home/max/Desktop/Tirocinio_Iris/Miche_Individuation/SamsungGalaxyS4/068_GS4_OU_R_RI_01_4.jpg_finalMask.png", 0);
	float score_mio = F1_Score(&GTmask, &mask_mio);
	cout << "mio = " << score_mio << endl;
	//Mat mask_krup = imread("/home/max/Desktop/Test/Test Finali/Krupicka/Utiris/069/IMG_069_R_1.defects.png", 0);
	Mat mask_krup = imread("/home/max/Desktop/Test/Test Finali/Krupicka/Miche/SamsungGalaxyS4_KrupickaOutput/068_GS4_OU_R_RI_01_4.defects.png", 0);
	float score_krup = F1_Score(&GTmask, &mask_krup);
	cout << "krupicka = " << score_krup << endl;

	float acc_mio = accuracy(&GTmask, &mask_mio);
	cout << "mio accuracy = " << acc_mio << endl;

	float acc_krup = accuracy(&GTmask, &mask_krup);
	cout << "krup accuracy = " << acc_krup << endl;*/
/*}*/




























