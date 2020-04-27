#include "IrisSegm.hpp"

IrisSegm::IrisSegm(std::string pathToDB) : eye(Eye(pathToDB))
{
}

IrisSegm::~IrisSegm()
{
}

void IrisSegm::run()
{
    auto[width, height] = obtain_w_h(eye.getEyeImg()->cols, eye.getEyeImg()->rows);
    std::cout << "ao, il primo è " << width << " , il secondo è " << height << std::endl;
    eye.resize(width, height);
    
    Preprocessing refCor(&eye);
    refCor.run();
    Segmentation daug(&eye);
    daug.run();
}

Eye* IrisSegm::getEye(){ return &eye; }