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
}

Eye* IrisSegm::getEye(){ return &eye; }