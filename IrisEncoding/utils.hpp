#include <boost/filesystem.hpp>
#define BOOST_THREAD_USE_LIB
#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED 
#include <iostream>

void get_all(const boost::filesystem::path& root, const std::string& ext, std::vector<boost::filesystem::path>& ret, std::vector<std::string>& names);