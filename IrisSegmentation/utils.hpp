#pragma once
#define BOOST_THREAD_USE_LIB
#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED 
#include <boost/filesystem.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>
#include <math.h>
#include "Eye.hpp"

using namespace std;
using namespace boost::filesystem;

void get_all(const boost::filesystem::path& root, const string& ext, vector<boost::filesystem::path>& ret, vector<std::string>& names);

pair<int,int> obtain_w_h(int cols, int rows);

int getIndexOfZeros(vector<int> line_int);

template<typename T> vector<T> slice(vector<T> &v, int m, int n)
{
    vector<T> vec;
    copy(v.begin() + m, v.begin() + n + 1, back_inserter(vec));
    return vec;
}