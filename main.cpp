#include "main.h"


int main(int argc, char *argv[]){
	string db_path, out_path, haar_casc;
	ifstream file("./init.txt");
	vector<string> tokens;
	if(file.is_open()){
		string line;
		while(getline(file, line)){
			tokens = split(line, '=');
			if(tokens[0]=="Out_Path")
				out_path = tokens[1];
			else if(tokens[0]=="DB_Path")
				db_path = tokens[1];
			else if(tokens[0]=="Viola_Jones")
				haar_casc = tokens[1];
			else{
				printf("Some error occured with the init values\n");
				exit(0);
			}
		}
	}
	else{
		printf("Cannot open file \n");
		exit(0);
	}
	file.close();
	int test_mode, i_db;
	cout << "Insert 1 for test mode: ";
	cin >> test_mode;
	if(test_mode == 1){
		test();
		return 0;
	}
	cout << "Insert 1 if dataset contains periocular images, 2 if dataset contains faces (to use viola-jones algorithm), 3 for NOReflection Mode: ";
	cin >> i_db;
	if(i_db == 1)
		run_Utiris(db_path, out_path);
	else if(i_db == 2){
		run_MICHE(db_path, out_path, haar_casc);
	}
	else if(i_db == 3){
		run_NORefl(db_path, out_path);
	}
	return 0;
}


/*
 * main per il database Utiris. In generale, per individuazione dell'iride in immagini contenenti zone perioculari.
 */
int run_Utiris(string db_path, string out_path){
	vector<string> vec_paths = create_dirs(db_path, out_path);
	
	auto start = std::chrono::system_clock::now();
	
	#pragma omp parallel for
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



		int r_min = height/10;
		int r_max = height/3;
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
		
		int r_pup_min = res->radius / 4;
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
		

		vector<string> splitted = split(vec_paths[i], '/');
		string destinazione = splitted[splitted.size()-2];
		string name = splitted[splitted.size()-1];
		cout << out_path+destinazione+"/"+name << endl;
		imwrite(out_path+destinazione+"/"+name, img_color);
		
		Mat bin_mask = binaryMask(&img_color, new_radius, center, radius_pup, center_pup);
		Mat out_mask;
		imgToNorm.copyTo(out_mask, bin_mask);
		
		map<string, Point> mappa;
		Mat out_img = normalizza(&imgToNorm, new_radius, center.x, center.y, radius_pup, center_pup.x, center_pup.y, &mappa);
		
		for(map<string,Point>::iterator it=mappa.begin(); it!=mappa.end(); ++it){
			imgToNorm.at<Vec3b>(it->second.y, it->second.x) = {255,255,255};
		}
		Mat norm_bgr[3];
		Mat normResized_new;
		resize(out_img, normResized_new, Size(600,100));
		split(out_img, norm_bgr);
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
		Mat upperEyelidMask = upperEyelidDetection(&norm_bgr[2], out_path+destinazione2);
		Mat lowerEyelidMask = lowerEyelidDetection(&norm_bgr[2]);
		Mat ultimateMask = Mat(lowerEyelidMask.rows, lowerEyelidMask.cols, CV_8UC1, Scalar(255));
		Mat reflectionMask = threshReflectionDetection(&norm_bgr[0], 5, -15);
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
		imwrite(out_path+destinazione+"/"+name+"_norm.png", normResized);
		imwrite(out_path+destinazione+"/"+name+"_maskRicomposta.png", bin_mask);
		imwrite(out_path+destinazione+"/"+name+"_ultimateMask.png", maskResized);

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


int run_NORefl(string db_path, string out_path){
	vector<string> vec_paths = create_dirs(db_path, out_path);
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



		int r_min = height/10;
		int r_max = height/3;
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
		circle(img_color, center, new_radius, Scalar(0,0,255), 1);
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
		circle(img_color, center_pup, radius_pup, Scalar(0,0,255), 1);
		printf("value_pup = %f, radius_pup = %d, center_pup(x,y) = %d,%d \n", res_pup->value, res_pup->radius, res_pup->center.x, res_pup->center.y);
		
		Mat bin_mask = binaryMask(&img_color, new_radius, center, radius_pup, center_pup);
		Mat out_mask;
		imgToNorm.copyTo(out_mask, bin_mask);
		
		map<string, Point> mappa;
		Mat out_img = normalizza(&imgToNorm, new_radius, center.x, center.y, radius_pup, center_pup.x, center_pup.y, &mappa);
		
		for(map<string,Point>::iterator it=mappa.begin(); it!=mappa.end(); ++it){
			imgToNorm.at<Vec3b>(it->second.y, it->second.x) = {255,255,255};
		}
		Mat norm_bgr[3];
		Mat normResized_new;
		resize(out_img, normResized_new, Size(600,100));
		split(out_img, norm_bgr);
		vector<string> splitted = split(vec_paths[i], '/');
		string destinazione = splitted[splitted.size()-2];
		string name = splitted[splitted.size()-1];
		imwrite(out_path+destinazione+"/"+name, img_color);
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
		Mat upperEyelidMask = upperEyelidDetection(&norm_bgr[2], out_path+name);
		Mat lowerEyelidMask = lowerEyelidDetection(&norm_bgr[2]);
		Mat ultimateMask = Mat(lowerEyelidMask.rows, lowerEyelidMask.cols, CV_8UC1, Scalar(255));
		for(int y = 0; y < ultimateMask.rows; y++){
			for(int x = 0; x < ultimateMask.cols; x++){
				if(lowerEyelidMask.at<uchar>(y,x) == 0 || upperEyelidMask.at<uchar>(y,x) == 0){
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
		imwrite(out_path+destinazione+"/"+name+"_norm.png", normResized);
		imwrite(out_path+destinazione+"/"+name+"_maskRicomposta.png", bin_mask);
		imwrite(out_path+destinazione+"/"+name+"_ultimateMask.png", maskResized);

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


int run_MICHE(string db_path, string out_path, string haar_casc){
	vector<string> vec_paths = create_dirs(db_path, out_path);
	auto start = std::chrono::system_clock::now();

	for( int i = 0; i < vec_paths.size(); i++){
		vector<string> splitted_1 = split(vec_paths[i], '/');
		string destinazione_1 = splitted_1[splitted_1.size()-2];
		string name_1 = splitted_1[splitted_1.size()-1];
		Mat img_in = imread(vec_paths[i], 1);
		Mat img_in_copy = img_in.clone();

		cout << "start with " << vec_paths[i] << endl;
		Mat inp_img;
		Mat mask_prev;
		search_reflection(&img_in, &mask_prev, 3, -20);
		inpaint_reflection(&img_in, &mask_prev, &inp_img, 1);

		Mat grayscaled;
		cvtColor(img_in, grayscaled, COLOR_BGR2GRAY);
		//Mat inp_can;
		CascadeClassifier casc(haar_casc);
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

		rectangle(img_in_copy, rec, Scalar(0,0,255), 3);
		roiMat = img_in(rec);
		cout << vec_paths[i] << endl;

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



		int r_min = height/10; // /6 per utiris, /8 per miche
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
		
		results* res = apply_daugman_operator( img, r_min, r_max);

		int new_radius = res->radius*(img_color.cols/width);
		Point center(res->center.x*(img_color.cols/width), res->center.y*(img_color.rows/height));
		circle(img_in, Point(center.x+rec.x,center.y+rec.y), new_radius, Scalar(0,0,255), 2);
		printf("value = %f, radius = %d, center(x,y) = %d,%d \n", res->value, res->radius, res->center.x, res->center.y);
		
		int r_pup_min = res->radius / 6;
		if( r_pup_min < 5) r_pup_min = 5;
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
		imwrite(out_path+destinazione+"/"+name, img_in);
		
		Mat finalMask = binaryMask(&inp_img, new_radius, Point(center.x+rec.x,center.y+rec.y), radius_pup, Point(center_pup.x+rec.x,center_pup.y+rec.y));

		Mat bin_mask = binaryMask(&img_color, new_radius, center, radius_pup, center_pup);
		
		map<string, Point> mappa;
		Mat out_img = normalizza(&imgToNorm, new_radius, center.x, center.y, radius_pup, center_pup.x, center_pup.y, &mappa);

		string destinazione2 = vec_paths[i].substr(vec_paths[i].length()-20, vec_paths[i].length()-1);
		string folder = destinazione2.substr(1, 3);
		cout << destinazione2 << endl;
		cout << vec_paths[i] << endl;
		Mat norm_bgr[3];
		split(out_img, norm_bgr);
		Mat upperEyelidMask = upperEyelidDetection(&norm_bgr[2], destinazione+"/"+name);
		cout << "upper" << endl;
		Mat lowerEyelidMask = lowerEyelidDetection(&norm_bgr[2]);
		cout << "lower" << endl;
		Mat ultimateMask = Mat(lowerEyelidMask.rows, lowerEyelidMask.cols, CV_8UC1, Scalar(255));
		Mat reflectionMask = threshReflectionDetection(&norm_bgr[0], 5, -15);
		cout << "reflection" << endl;

		for(int y = 0; y < ultimateMask.rows; y++){
			for(int x = 0; x < ultimateMask.cols; x++){
				if(lowerEyelidMask.at<uchar>(y,x) == 0 || upperEyelidMask.at<uchar>(y,x) == 0 || reflectionMask.at<uchar>(y,x) != 0){
					ultimateMask.at<uchar>(y,x) = 0;
					Point pt = mappa[to_string(x)+","+to_string(y)];
					bin_mask.at<uchar>(pt.y, pt.x) = 0;
					finalMask.at<uchar>(rec.y+pt.y, rec.x+pt.x) = 0;
				}
			}
		}
		Mat maskResized;
		resize(ultimateMask, maskResized, Size(600, 100));
		Mat normResized;
		resize(out_img, normResized, Size(600,100));
		imwrite(out_path+destinazione+"/"+name+"_finalMask.png", finalMask);
		imwrite(out_path+destinazione+"/"+name+"_norm.png", normResized);
		imwrite(out_path+destinazione+"/"+name+"_maskRicomposta.png", bin_mask);
		imwrite(out_path+destinazione+"/"+name+"_ultimateMask.png", maskResized);

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

vector<string> readFile(string strFile, ifstream &iFile) //Read the file into the vector function definition
{
	vector<string> vecLines;
	string line; // line of the file
	iFile.open(strFile.c_str()); //Opens file
    while(getline(iFile, line)){ //read data from file object and put it into string.
    	vecLines.push_back(line);
    }
	iFile.close(); //Close the input file
	return vecLines;
}

int test_miche(){
	ifstream inFile; // file di input
	vector<string> names = readFile("/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/MICHE-II_masks_labels.txt", inFile);
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
		Mat GTmask = imread("/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/masks/ground_truth/"+names[i]+".defects.png", 0);
		string maskName = names[i];
		cout << maskName << endl;
		//string mio = maskName;
	//	mio+=".jpg_finalMask.png";
		//cout << mio << endl;
		Mat mask_mio = imread("/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/masks/rielaborazione_micheII_output/"+maskName+".jpg_finalMask.png", 0);
	//	Mat mask_mio = imread("/home/max/Desktop/Univ/ground_truth/Utiris_MioAlgo/"+maskName+".JPG_maskRicomposta.png", 0);
	//	Mat mask_mio = imread("/home/max/Desktop/Univ/Tesi/MioAlgo/"+maskName+"_finalMask.png", 0);
		Mat mask_krup = imread("/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/masks/krupicka/"+maskName+".defects.png", 0);
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
}


int test(){
	string krup_path = "/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/UBI/ubiris_krupicka/";
	string rielab_path = "/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/UBI/ubiris_output/input/";
	string mask_path = "/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/UBI/ubiris_masks/";
	vector<string> names = getFileNames("/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/UBI/ubiris_masks");
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
		Mat GTmask = imread(mask_path+names[i], 0);
		string maskName = names[i];	
		cout << maskName << endl;
		string subName = split(names[i], '.')[0];
		//string mio = maskName;
	//	mio+=".jpg_finalMask.png";
		//cout << mio << endl;
		Mat mask_mio = imread(rielab_path+maskName+"_maskRicomposta.png", 0);
	//	Mat mask_mio = imread("/home/max/Desktop/Univ/ground_truth/Utiris_MioAlgo/"+maskName+".JPG_maskRicomposta.png", 0);
	//	Mat mask_mio = imread("/home/max/Desktop/Univ/Tesi/MioAlgo/"+maskName+"_finalMask.png", 0);
		Mat mask_krup = imread(krup_path+subName+".defects.png", 0);
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
}




/**
 * @brief creo le directory presenti in inp_path all'interno di out_path e ritorno i rispettivi path (ritorno out_vec che contiene tali path). N.B.: valido per Utiris
 * @param inp_path path alla directory di input (in cui si trova il DB, in questo caso Utiris)
 * @param out_path path alla directory di output in cui andranno a crearsi tante dirs quante sono in inp_path
 * @return vector<string> out_vec contenente i path alle singole directory create
 */
vector<string> create_dirs(string inp_path, string out_path){ // per Utiris_DB
	char const *inp = inp_path.c_str();
	char const *out = out_path.c_str();
	vector<string> out_vec;
	DIR *inp_dir; DIR *out_dir; DIR *eye_dir;
	struct dirent *ent;
	struct dirent *eye_file;
	int check;
	char filename[256]; char* extension; char file_n[256]; char eye_path[256];
	if ((inp_dir = opendir(inp)) != NULL && (out_dir = opendir(out)) != NULL) {
		while ((ent = readdir (inp_dir)) != NULL) {
			if(ent->d_name[0] == '.') continue;
			extension = ent->d_name;
			strncpy(filename, out, sizeof(filename)); 			// non faccio controlli dkw
			strncpy(file_n, inp, sizeof(file_n));
			strncat(filename, extension, (sizeof(filename) - strlen(filename)));		// non faccio controlli dkw
			strncat(file_n, extension, (sizeof(file_n) - strlen(file_n)));

			check = mkdir(filename, 0777);
			if(check){cout << "errore nella creazione della cartella " << ent->d_name << endl; break;}
			strncat(file_n, "/", (sizeof(file_n) - strlen(file_n)));
			eye_dir = opendir(file_n);
			while ((eye_file = readdir (eye_dir)) != NULL){
				if(eye_file->d_name[strlen(eye_file->d_name)-1] == 'f' || eye_file->d_name[strlen(eye_file->d_name)-1] == 'G' || eye_file->d_name[strlen(eye_file->d_name)-1] == 'g'){		// controllo solo se è JPG ma per ora mi basta solo la G
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