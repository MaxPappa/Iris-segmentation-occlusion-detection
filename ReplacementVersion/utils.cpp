#include "utils.hpp"

// return the filenames of all files that have the specified extension
// in the specified directory and all subdirectories
void get_all(const path& root, const string& ext, vector<path>& ret)
{
    if(!exists(root) || !is_directory(root)) return;

    recursive_directory_iterator it(root);
    recursive_directory_iterator endit;

    while(it != endit)
    {
        if(is_regular_file(*it) && it->path().extension() == ext)
            ret.push_back(it->path().parent_path()/it->path().filename());
        ++it;
    }
}

pair<int,int> obtain_w_h(int cols, int rows){
	//if(cols/128 <= 0) return pair<int,int>(1,1);
	int h = cols/256;
	int k = h;
	while(cols%k != 0 || rows%k != 0){
		k--;
	}
	pair<int,int> coppia(cols/k, rows/k);
	return coppia;
}

int getIndexOfZeros(vector<int> lineInt){
	for( int i = 0; i < lineInt.size(); i++ ){
		if( lineInt[i] == 0 ) return i;
	}
	return -1;
}