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

    vector<boost::filesystem::path> ret;
    string ext{".JPG"};
    string path{"/home/max/Desktop/Univ/Magistrale/Biometrics_Systems/Utiris_inpout/RGB_Images/032/"};
    boost::filesystem::path root{path}; // root folder of the dataset
    get_all(root, ext, ret);

    boost::timer::auto_cpu_timer t; // when destructor is called the elapsed time is printed
    
    #pragma omp parallel for shared(ret) num_threads(8)
    for(int i = 0; i < ret.size(); i++){
        IrisSegm irSe(ret[i].string());
        cv::Mat img = *(irSe.getEye()->getImg());
       // cv::namedWindow("Image Window");

        irSe.run();
        
        cv::Mat immagine = *(irSe.getEye()->getImg());
        int c = round(immagine.cols/irSe.getEye()->getImgWidth());     // i'm not sure if every x,y pair should have the same coefficient c (!)
        cv::circle(immagine, irSe.getEye()->getIrisCenter()*c, irSe.getEye()->getIrisRadius()*c, cv::Scalar(0,0,255), 3);
        int xRoi = irSe.getEye()->getIrisCenter().x - irSe.getEye()->getIrisRadius();
        int yRoi = irSe.getEye()->getIrisCenter().y - irSe.getEye()->getIrisRadius();
        
        cv::Point centerPup(xRoi+irSe.getEye()->getPupilCenter().x, yRoi+irSe.getEye()->getPupilCenter().y);
        cv::circle(immagine, centerPup*c, irSe.getEye()->getPupilRadius()*c, cv::Scalar(255,0,0), 3);

       // cv::imshow("Image Window", immagine);
       // cv::waitKey(0);

       /* cv::Mat pupilROI = *(irSe.getEye()->getPupilROI());
        cv::namedWindow("Show Pupil");
        cv::imshow("Show Pupil", pupilROI);
        cv::waitKey(0);

        cv::namedWindow("Show Mask");
        cv::imshow("Show Mask", *(irSe.getEye()->getBinMask()));
        cv::waitKey(0);

        cv::destroyAllWindows();*/

        immagine.release();
    }
}