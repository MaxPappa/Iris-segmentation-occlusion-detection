#include "recognitionMain.hpp"

int main(int argc, char** argv)
{
    if(argc == 2 && strcmp(argv[1], "--help")==0)
    {
        std::string help = "This is an LBP Encoder.\n\n";
        help+= "The only argument to give is a folder path.\n\tThis path should contain normalized iris' and their masks.";
        std::cout << help << std::endl;
        return 0;
    }
    //std::list<Subject*> subList;

    boost::filesystem::path dstFolder(boost::filesystem::current_path().string()+"/LBP_Codes");
    if(!(boost::filesystem::exists(dstFolder)))
    {
        boost::filesystem::create_directory(dstFolder);
    }
    else{
        std::cout << "The folder LBP_Codes already exists." << std::endl;
        return 0;
    }
    std::vector<boost::filesystem::path> ret;
    std::vector<std::string> names;
    std::string ext{".JPG"};
    std::string path{argv[1]};
    boost::filesystem::path root{path}; // root folder of the dataset
    get_all(root, ext, ret, names);

   // #pragma omp parallel for shared(names) num_threads(4)
    for(int i = 0; i < ret.size(); i++){
        std::cout << names[i] << " is starting now"<< std::endl;
        
        Subject sub(path+names[i]+"_NORMALIZED.JPG", path+names[i]+"_NORMMASK.JPG", names[i]);
        LBP featEx = LBP(&sub);
        featEx.encode();
        featEx.encodeSave();
    }
}
