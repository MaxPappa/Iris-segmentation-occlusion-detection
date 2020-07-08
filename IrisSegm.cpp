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



int main()
{
    boost::filesystem::path dstFolder(current_path().string()+"/Utiris");
    if(!(boost::filesystem::exists(dstFolder)))
    {
        std::cout << "ao" << std::endl;
        boost::filesystem::create_directory(dstFolder);
    }
    else{
        std::cout << "Do you want to overwrite the previous run? Y/N" << std::endl;
        char ans;
        std::cin >> ans;
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
                std::cout << "Try again, i can't understand" << std::endl;
                break;
            }
        }
    }
    std::cout << "mannagg" << std::endl;
    vector<boost::filesystem::path> ret;
    vector<std::string> names;
    string ext{".JPG"};
    string path{"/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/Utiris_inpout/RGB_Images/"};
    boost::filesystem::path root{path}; // root folder of the dataset
    get_all(root, ext, ret, names);

    boost::timer::auto_cpu_timer t; // when destructor is called the elapsed time is printed
    
    #pragma omp parallel for shared(ret) num_threads(4)
    for(int i = 0; i < ret.size(); i++){
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

        imgNorm.release();
        binMask.release();
        normMask.release();
        image.release();
    }
}