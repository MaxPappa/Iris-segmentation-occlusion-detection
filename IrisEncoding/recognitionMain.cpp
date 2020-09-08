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
    boost::filesystem::path masksDstFolder(boost::filesystem::current_path().string()+"/Masks");
    if(!(boost::filesystem::exists(dstFolder)))
    {
        boost::filesystem::create_directory(dstFolder);
        boost::filesystem::create_directory(masksDstFolder);
    }
    else{
        std::cout << "The folder LBP_Codes already exists. Do you want to resume a run? Y/N" << std::endl;
        char ans;
        std::cin >> ans;
        std::tolower(ans);
        switch (ans)
        {
            case('y'):
            {
                std::cout << "Ok, resuming previously started run" << std::endl;
                break;
            }
            case('n'):
            {
                std::cout << "Ok, so this run is going to be aborted." << std::endl;
                return 0;
                break;
            }
            default:
            {
                std::cout << "Options are y or n, so try again." << std::endl;
                return 0;
                break;
            }
        }
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
