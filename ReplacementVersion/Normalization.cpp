#include "Normalization.hpp"

Normalization::Normalization(){}

Normalization::~Normalization(){}

cv::Mat Normalization::binaryMask(Eye* eye)
{
	int c = round(eye->getImg()->cols / eye->getImgWidth());
    int pupX = ((eye->getIrisCenter().x - eye->getIrisRadius()) + eye->getPupilCenter().x ) * c;
    int pupY = ((eye->getIrisCenter().y - eye->getIrisRadius()) + eye->getPupilCenter().y ) * c;
    cv::Mat mask = cv::Mat::zeros(cv::Size(eye->getImg()->cols, eye->getImg()->rows), CV_8UC1);
	circle(mask, eye->getIrisCenter()*c, eye->getIrisRadius()*c, cv::Scalar(255), -1); // thickness = -1 -> filled circle
	circle(mask, cv::Point(pupX,pupY), eye->getPupilRadius()*c, cv::Scalar(0), -1);
	return mask;
}



void Normalization::normalize(Eye* eye, std::map<size_t, cv::Point>* coords)
{
    int c = round(eye->getImg()->cols / eye->getEyeImg()->cols);
    int irisR =  eye->getIrisRadius() * c;
    int irisX = eye->getIrisCenter().x * c;
    int irisY = eye->getIrisCenter().y * c;

    int pupR = eye->getPupilRadius() * c;
    int pupX = ((eye->getIrisCenter().x - eye->getIrisRadius()) + eye->getPupilCenter().x ) * c;
    int pupY = ((eye->getIrisCenter().y - eye->getIrisRadius()) + eye->getPupilCenter().y ) * c;

    int height = irisR*2;
    int width = round(irisR * 2 * M_PI);
    cv::Mat norm = cv::Mat::zeros(height, width, CV_8UC3);
    double thetaStep = (2*M_PI)/width;
	float xp, yp, xi, yi;
	int x, y;
	int ind = 0;
    size_t seed;
    cv::Mat* imgPtr = eye->getImg();
    for(double i = 3*M_PI/2; i < 2*M_PI + 3*M_PI/2; i += thetaStep)
    {
		xp = (pupX + pupR * cos(i));
		yp = (pupY + pupR * sin(i));
		xi = (irisX + irisR * cos(i));
		yi = (irisY+ irisR * sin(i));
		for(int j = 0; j < height; j++)
        {
            seed = 0;
			float pas = (float)j/height;
			int x = round( ( 1 - pas ) * xi + pas * xp );
			int y = round( ( 1 - pas ) * yi + pas * yp );
			if ( x >= 0 && x < eye->getImg()->cols && y >= 0 && y < eye->getImg()->rows )
            {
				boost::hash_combine(seed, j);
                boost::hash_combine(seed, ind);
                norm.at<cv::Vec3b>(j,ind) = imgPtr->at<cv::Vec3b>(y,x);		// coordinate per avere upper eyelid al centro
				coords->insert(std::pair<size_t,cv::Point>(seed, cv::Point(x,y))); // maybe is better to use an hash function (i'll change this) (!)
            }
		}
		ind++;
	}
    imgPtr = 0;
    eye->setNormImg(&norm);
    norm.release();
}


void Normalization::run(Eye* eye)
{
    std::map<size_t, cv::Point> coords = std::map<size_t, cv::Point>();
    normalize(eye, &coords);
    eye->setEyeCoords(&coords);
    cv::Mat binMask = binaryMask(eye);
    eye->setBinMask(&binMask);

    binMask.release();
}