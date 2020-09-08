#include "IrisSegm.hpp"

IrisSegm::IrisSegm(std::string pathToDB) : eye(Eye(pathToDB))
{
}

IrisSegm::~IrisSegm()
{
}


void IrisSegm::run()
{
    Preprocessing refCor(&eye);
    refCor.run();

    Segmentation daug(&eye);
    daug.run();

    try{
        Normalization norm = Normalization();
        norm.run(&eye);
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    OcclusionDetector occDec;
    occDec.run(&eye);
}

Eye* IrisSegm::getEye(){ return &eye; }


// arguments should be: "y", "/path/to/dataset/", "/path/to/output/"
int main(int argc, char** argv)
{
    if(argc == 2 && strcmp(argv[1], "--help")==0)
    {
        std::string help = "Welcome to my IrisSegmentation project. This project has been created as a bachelor project and then improved in order to use it ";
        help+="as a project for Biometrics Systems course.\nPossible arguments (write them in order) are:\n\n\n";
        help+="y/n - if you have already ran this IrisSegmenter, y means that you want to overwrite previous results,";
        help+="\n\tn means that you don't want it to happen, and then nothing will be done.";
        help+="\n\tNOTICE: if this is your first run, or you want the output in a new folder (u should not create it manually), \n\tthen u can avoid to use this argument.\n\n\n";
        help+="path/to/dataset/ - this argument is a path to the folder containing the iris dataset.\n\n\n";
        help+="path/to/output/folder/ - this argument is a path to the output folder. Notice: if the folder doesn't exist, it will be created.\n";
        std::cout << help << std::endl;
        return 0;
    }
    if(argc >= 5)
    {
        std::cout << "Too many arguments given. Use the --help option in order to learn which are the possible arguments." << std::endl;
        return 0;
    }

    boost::filesystem::path dstFolder(current_path().string()+argv[3]);//"/Utiris_Segmented");
    if(!(boost::filesystem::exists(dstFolder)))
    {
        boost::filesystem::create_directory(dstFolder);
    }
    else{
        char ans = *argv[1];
        std::tolower(ans);
        switch (ans)
        {
            case('y'):
            {
                boost::filesystem::remove_all(dstFolder);
                boost::filesystem::create_directory(dstFolder);
                break;
            }
            case('n'):
            {
                std::cout << "Ok, so this run is going to be aborted." << std::endl;
                break;
            }
            default:
            {
                std::cout << "Options are y or n, so try again." << std::endl;
                break;
            }
        }
    }
    vector<boost::filesystem::path> ret;
    vector<std::string> names;
    string ext{".JPG"};
    string path{argv[2]};
    boost::filesystem::path root{path}; // root folder of the dataset
    get_all(root, ext, ret, names);

    boost::timer::auto_cpu_timer t; // when destructor is called the elapsed time is printed
    
    #pragma omp parallel for shared(ret) num_threads(4)
    for(int i = 0; i < ret.size(); i++){
        std::cout << names[i] << " is starting now"<< std::endl;
        IrisSegm irSe(ret[i].string());
        cv::Mat img = *(irSe.getEye()->getImg());
       // cv::namedWindow("Image Window");

        irSe.run();
        
        cv::Mat image = *(irSe.getEye()->getImg());
        int c = round(image.cols/irSe.getEye()->getImgWidth());     // i'm not sure if every x,y pair should have the same coefficient c (!)
        cv::circle(image, irSe.getEye()->getIrisCenter()*c, irSe.getEye()->getIrisRadius()*c, cv::Scalar(0,0,255), 3);
        int xRoi = irSe.getEye()->getIrisCenter().x - irSe.getEye()->getIrisRadius();
        int yRoi = irSe.getEye()->getIrisCenter().y - irSe.getEye()->getIrisRadius();
        
        cv::Point centerPup(xRoi+irSe.getEye()->getPupilCenter().x, yRoi+irSe.getEye()->getPupilCenter().y);
        cv::circle(image, centerPup*c, irSe.getEye()->getPupilRadius()*c, cv::Scalar(255,0,0), 3);

        cv::Mat imgNorm = *(irSe.getEye()->getNormImg());
        cv::Mat binMask = *(irSe.getEye()->getBinMask());
        cv::Mat normMask = *(irSe.getEye()->getNormMask());

        cv::imwrite(dstFolder.string()+"/"+names[i]+"_NORMALIZED"+ext, imgNorm);
        cv::imwrite(dstFolder.string()+"/"+names[i]+"_BINARYMASK"+ext, binMask);
        cv::imwrite(dstFolder.string()+"/"+names[i]+"_NORMMASK"+ext, normMask);
        cv::imwrite(dstFolder.string()+"/"+names[i]+"_IMAGE"+ext, image);

        std::cout << names[i] << " DONE!"<< std::endl;

        imgNorm.release();
        binMask.release();
        normMask.release();
        image.release();
    }
}