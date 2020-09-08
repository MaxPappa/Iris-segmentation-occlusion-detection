#include "utils.hpp"

void get_all(const boost::filesystem::path& root, const std::string& ext, std::vector<boost::filesystem::path>& ret, std::vector<std::string>& names)
{
    if(!exists(root) || !is_directory(root)) return;

    boost::filesystem::recursive_directory_iterator it(root);
    boost::filesystem::recursive_directory_iterator endit;

    while(it != endit)
    {
        size_t pivot;
        std::string filename = it->path().filename().string();
        if(is_regular_file(*it) && it->path().extension() == ext && filename.find("_NORMMASK") != std::string::npos){
            if(filename.find("_NORMMASK") != std::string::npos)
                pivot = filename.find("_NORMMASK");
            std::string basename = filename.substr(0, pivot);
            ret.push_back(it->path().parent_path()/it->path().filename());
			names.push_back(basename);
		}
        ++it;
    }
}   