

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>

using namespace cv;
using namespace std;

vector<Point> detectFourPoints(Mat src);