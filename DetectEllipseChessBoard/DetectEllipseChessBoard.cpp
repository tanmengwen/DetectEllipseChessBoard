
/************************************************************************/
//1.读取视频文件
//2.检测四个定位顶点坐标
//3.对棋盘格图像进行图像处理，取合适的阈值，采用mask对棋盘格区域处理
//4.对阈值图像进行轮廓查找
//5.找到轮廓判断轮廓个数，不正常则调整阈值
//6.对轮廓进行有序查找排列，并输出圆形的中心坐标，进行保存
/************************************************************************/

#include "stdafx.h"

#include "stdafx.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>
//#include <algorithm> 

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;


bool _start = false;
bool _resetCap = false;
bool _test = false;
bool _readVideo = true;
bool _fitEcllipseMethod = false;
bool _drawContours = true;

long totalFrameNumber;
VideoCapture cap;
Mat frame,grayImage,image,mask;

int thresholdsliderPos = 40;
Point lastp1,lastp2,lastpp;
int row =8;
int col = 8;

vector<Point> fourPoints;
vector<Point2f> sortedPoints;
vector<vector<Point2f>> allPoints;

void mouseEvent(int event, int x, int y, int flags, void *param );
bool generateMask(vector<Point> points);
void processImage(int, void*);
int sortX(Point2f p1,Point2f  p2);
int sortY(Point2f p1,Point2f  p2);
Point lineEquation(int totalNum,int num,Point p1, Point p2);
vector<Point> getgoodFeaturesFourPoints(Mat src);
vector<Point> getFitPointSize(int row,int col,vector<Point> points);
void sortPoint(int row,int col,vector<Point2f> findpoints,vector<Point> points,bool bPickPoint);
void saveData(const char filename[]);

int _tmain(int argc, _TCHAR* argv[])
{
	if (_readVideo)
	{
		cap = VideoCapture("video.avi");
		if(!cap.isOpened())  // check if we succeeded
			return -1;
		totalFrameNumber = cap.get(CV_CAP_PROP_FRAME_COUNT);
		cout<<"totalFrameNumber "<<totalFrameNumber<<endl;
	}else{
		totalFrameNumber = 10000;
	}


	for(int i=0;i<totalFrameNumber;i++)
	{
		int key = waitKey(20);
		if (_readVideo)
		{
			cap >> frame; // get a new frame from camera
			cvtColor(frame, grayImage, CV_BGR2GRAY);

			fourPoints = getgoodFeaturesFourPoints(frame);
			for (int i=0;i<fourPoints.size();i++)
			{
				circle(frame,fourPoints[i],2.5f, Scalar(0,0,255),2);
			}

			vector<Point> p = getFitPointSize(row,col,fourPoints);
			bool success = generateMask(p);
			if (success)
			{
				image = Mat::zeros(frame.size(),CV_8UC1);
				grayImage.copyTo(image,mask);
				createTrackbar( "threshold", "result", &thresholdsliderPos, 255, processImage );
				processImage(0, 0);
			}

			cout<<"i "<<i<<" points "<<fourPoints.size() <<endl;
			imshow("frame",frame);
		} 
		else
		{
			frame = imread("image119.jpg");
			cvtColor(frame, grayImage, CV_BGR2GRAY);
			namedWindow("frame");
			setMouseCallback("frame",mouseEvent);
			
			for (int i=0;i<fourPoints.size();i++)
			{
				circle(frame,fourPoints[i],2.5f, Scalar(0,0,255),2);
			}
			bool success = generateMask(fourPoints);
			if (success)
			{
				grayImage.copyTo(image,mask);
				createTrackbar( "threshold", "result", &thresholdsliderPos, 255, processImage );
				processImage(0, 0);
			}

			imshow("frame",frame);
		}
		
		



		if (key == 't')
		{
			if (_test)
			{
				_test = false;
			} 
			else
			{
				_test = true;
			}
		}
		if (key == 's')
		{
			if (_start)
			{
				_start = false;
			} 
			else
			{
				_start = true;
			}
		}
		if (key == 'r')
		{

		}



		//stringstream ss;
		//string str;
		//ss<<i;
		//ss >> str;
		//string  name = "image/image" + str +".jpg";


		if(key == 27) break;
	}

	saveData( "data.txt");

	waitKey(0);
	return 0;
}

//mouse event callback  
//pick four points of the chess board
void mouseEvent(int event, int x, int y, int flags, void *param )  
{  
	if (event==CV_EVENT_LBUTTONDOWN)   
	{  
		fourPoints.push_back(Point(x,y));
	}  
}

void processImage(int, void*)
{
	vector<vector<Point> > contours;
	vector<Point2f>  centers;

	Mat bimage;
	threshold(image,bimage,thresholdsliderPos,255,THRESH_BINARY);  //THRESH_BINARY  THRESH_TOZERO

	imshow("bimage", bimage);
	//imwrite("bimage.jpg",bimage);

	findContours(bimage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	for(size_t i = 0; i < contours.size(); i++)
	{
		size_t count = contours[i].size();
		if( count < 6 )
			continue;

		double area0 = contourArea(contours[i]);
		if (area0> 50 && area0 < 2000)
		{
			Mat pointsf;
			Mat(contours[i]).convertTo(pointsf, CV_32F);

			if( contours[i].size() > 20)
			{
				if (_fitEcllipseMethod)
				{
					RotatedRect box = fitEllipse(pointsf);
					if( MAX(box.size.width, box.size.height) > MIN(box.size.width, box.size.height)*30)
						continue;

					ellipse(frame, box, Scalar(0,0,255), 0.1f, 4);
					//ellipse(frame, box.center, box.size*0.5f, box.angle, 0, 360, Scalar(0,255,0), 0.2f, CV_AA);
					circle(frame,box.center,0.5f, Scalar(0,255,0),0.1f);
					centers.push_back(box.center);
				} 
				else
				{
					Moments mu;
					mu = moments( contours[i], false ); 
					Point2f p = Point2f( static_cast<float>(mu.m10/mu.m00) , static_cast<float>(mu.m01/mu.m00) ); 
					circle(frame,p,0.3f, Scalar(0,255,255),0.4f);
					centers.push_back(p);
				}
				
				if (_drawContours)
				{
					drawContours(frame, contours, (int)i,  Scalar(255,0,0), 0.2f, 8);
				}
			}
		}
	}

	if (centers.size()>0)
	{
		sortPoint(row,col,centers,fourPoints,!_readVideo);
	}

	if (sortedPoints.size()==row*col)
	{
		allPoints.push_back(sortedPoints);
	}else
	{
		cout<<"process failed"<<endl;
	}

	sortedPoints.clear();

	imshow("result", frame);
}


Point lineEquation(bool bFit,int totalNum,int num,Point p1, Point p2){

	if (bFit)
	{
		Point p= p1 - p2;

		float a = (float)(p.x*p.x+p.y*p.y);
		float width = sqrt(a);

		Point distance = Point(p.x*0.5f/totalNum,p.y*0.5f/totalNum);
		Point pp1,pp2;
		pp1 = Point(p1.x-distance.x,p1.y-distance.y);
		distance = Point(p.x*(totalNum-0.5f)/totalNum,p.y*(totalNum-0.5f)/totalNum);
		pp2 = Point(p1.x-distance.x,p1.y-distance.y);

		p= pp1 - pp2;
		distance = Point(p.x*num/(totalNum-1),p.y*num/(totalNum-1));

		if (num==1)
		{
			lastpp = pp1;
		}

		return Point(pp1.x-distance.x,pp1.y-distance.y);
	} 
	else
	{
		Point p= p1 - p2;

		float a = (float)(p.x*p.x+p.y*p.y);
		float width = sqrt(a);

		Point distance = Point(p.x*num/totalNum,p.y*num/totalNum);

		return Point(p1.x-distance.x,p1.y-distance.y);
	}
}

int sortX(Point2f p1,Point2f  p2)
{
	return p1.x < p2.x;
}
int sortY(Point2f p1,Point2f  p2)
{
	return p1.y < p2.y;
}

static Scalar random_color()
{
	RNG rng(0xFFFFFFFF);
	int icolor = (unsigned)rng;

	return Scalar(icolor&0xFF, (icolor>>8)&0xFF, (icolor>>16)&0xFF);
}

vector<Point> getgoodFeaturesFourPoints(Mat src)
{

	vector<Point> corners;
	double qualityLevel = 0.01;
	double minDistance = 10;
	int blockSize = 3;
	bool useHarrisDetector = false;
	double k = 0.04;

	Mat image;
	src.copyTo(image);
	cvtColor(image,image,CV_BGR2GRAY);
	Mat bimage;
	threshold(image,bimage,200,255,THRESH_BINARY);
	/// Apply corner detection :Determines strong corners on an image.
	goodFeaturesToTrack( bimage, 
		corners,
		4,
		qualityLevel,
		minDistance,
		Mat(),
		blockSize,
		useHarrisDetector,
		k );

 
	/*for( int i = 0; i < corners.size(); i++ ){   
		circle( image,  corners[i], 5,  Scalar(255), 2, 8, 0 );   
		circle( image, corners[i], 4, Scalar(0,255,0), 2, 8, 0 );   
	}*/ 
	//imwrite("Features.jpg",image);

	vector<Point> p;
	for (int i =0;i<corners.size();i++)
	{
		p.push_back(corners[i]);
	}

	vector<Point> fourPoints;
	Point p1,p2,p3,p4;
	sort(p,sortX);
	if (p[0].y<p[1].y)
	{
		p1 = p[0];
		p4 = p[1];
	}else
	{
		p1 = p[1];
		p4 = p[0];
	}
	if (p[2].y<p[3].y)
	{
		p2 = p[2];
		p3 = p[3];
	}else
	{
		p2 = p[3];
		p3 = p[2];
	}

	fourPoints.push_back(p1);
	fourPoints.push_back(p2);
	fourPoints.push_back(p3);
	fourPoints.push_back(p4);
	return fourPoints;
}

//图像提取的四个点，进行合适区域的剪切
vector<Point> getFitPointSize(int row,int col,vector<Point> points)
{
	//p1-p4   p2-p3
	vector<Point> p;
	for (int j= 1;j<row+1;j++)
	{
		Point p1 = lineEquation(true,row+1,j,points[0],points[3]);
		if (j==1)
		{
			lastp1=lastpp;
			p.push_back(lastpp);
		}
		Point p2 = lineEquation(true,row+1,j,points[1],points[2]);
		if (j==1)
		{
			p.push_back(lastpp);
			lastp2=lastpp;
		}

		//line(frame,p1,p2,Scalar(255, 0, 255),0.3f,4);

		if (j==8)
		{
			p.push_back(p2);
			p.push_back(p1);
		}
	}
	//p1-p2   p4-p3
	vector<Point> pp;
	for (int j= 1;j<col+1;j++)
	{
		Point p1 = lineEquation(true,col+1,j,p[0],p[1]);
		if (j==1)
		{
			pp.push_back(lastpp);
		}
		Point p2 = lineEquation(true,col+1,j,p[3],p[2]);
		if (j==1)
		{
			pp.push_back(lastpp);
		}

		//line(frame,p1,p2,Scalar(255, 0, 255),0.3f,4);

		if (j==8)
		{
			pp.push_back(p2);
			pp.push_back(p1);
		}
	}

	return pp;
}

//按行，查找到行区域块,对所有的点进行重排序
void sortPoint(int row,int col,vector<Point2f> findpoints,vector<Point> points,bool bPickPoint)
{
	vector<vector<Point>> vectorContour;

	//line(frame,lastp1,lastp2,Scalar(255, 0, 255),0.3f,4);
	if (bPickPoint)
	{
		Point lastp1= points[0],lastp2= points[1];
		//line(frame,lastp1,lastp2,Scalar(255, 0, 255),0.3f,4);
		for (int j= 1;j<row+1;j++)
		{
			Point p1 = lineEquation(false,row,j,points[0],points[3]);
			Point p2 = lineEquation(false,row,j,points[1],points[2]);

			vector<Point> contour;
			contour.push_back(lastp1);    lastp1 = p1;
			contour.push_back(lastp2);    lastp2 = p2;
			contour.push_back(p1);
			contour.push_back(p2);
			vectorContour.push_back(contour);
		}
	} 
	else
	{
		for (int j= 1;j<row+1;j++)
		{
			Point p1 = lineEquation(true,row+1,j,points[0],points[3]);
			if (j==1)
			{
				lastp1=lastpp;
			}
			Point p2 = lineEquation(true,row+1,j,points[1],points[2]);
			if (j==1)
			{
				lastp2=lastpp;
			}

			//line(frame,p1,p2,Scalar(255, 0, 255),0.3f,4);

			vector<Point> contour;
			contour.push_back(lastp1);    lastp1 = p1;
			contour.push_back(lastp2);    lastp2 = p2;
			contour.push_back(p1);
			contour.push_back(p2);
			vectorContour.push_back(contour);
		}
	}


	//line(frame,lastp1,lastp2,Scalar(255, 0, 255),0.3f,4);

	//判断查找到的点是否在相应的行区域块里面
	vector<Point2f> findPointsCopy;
	vector<Point2f> singleLineSortedPoints;
	for (int i=0;i<findpoints.size();i++)
	{
		findPointsCopy.push_back(findpoints[i]);
	}

	for (int i=0;i<vectorContour.size();i++)
	{
		vector<Point2f> pointsCollected;

		Mat src = Mat::zeros( frame.size(), CV_8UC1 );
		//画行区域轮廓
		line(src,vectorContour[i][0],vectorContour[i][1],Scalar( 255 ),1,4);
		line(src,vectorContour[i][1],vectorContour[i][3],Scalar( 255 ),1,4);
		line(src,vectorContour[i][3],vectorContour[i][2],Scalar( 255 ),1,4);
		line(src,vectorContour[i][2],vectorContour[i][0],Scalar( 255 ),1,4);

		//得到轮廓
		vector<vector<Point> > contours; vector<Vec4i> hierarchy;
		Mat src_copy = src.clone();
		findContours( src_copy, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

		for(int j=0;j<findPointsCopy.size();j++)
		{
			if (pointPolygonTest( contours[0], findPointsCopy[j], false )>0)
			{
				pointsCollected.push_back(findPointsCopy[j]);
			}
		}
		//按照x坐标，从小到大排序
		sort(pointsCollected.begin(),pointsCollected.end(),sortX);
		for (int k=0;k<pointsCollected.size();k++)
		{
			sortedPoints.push_back(pointsCollected[k]);
			//circle(frame,pointsCollected[k],3, Scalar(0,20*k,255),2,8);
		}

		pointsCollected.clear();
	}

	//给找到的椭圆进行数字标记
	for( int i = 0; i < sortedPoints.size(); i++ )
	{
		char imageName[256];
		sprintf_s(imageName, "%d", i);
		putText( frame, imageName, sortedPoints[i],CV_FONT_HERSHEY_COMPLEX, 0.6f, Scalar(255, 0, 0) );

		if (i<sortedPoints.size()-1)
		{
			//line(frame,sortedPoints[i],sortedPoints[i+1],Scalar(255, 0, 255),0.3f,4);
		}
	}
}

//点取四点后生成Mask
bool generateMask(vector<Point> points){
	if (points.size()==4)
	{
		mask = Mat::zeros(frame.size(), CV_8UC1);

		Point rook_points[1][4]; 
		rook_points[0][0] = points[0];  
		rook_points[0][1] = points[1];  
		rook_points[0][2] = points[2];  
		rook_points[0][3] = points[3];  
		const Point* ppt[1] = { rook_points[0] };  
		int npt[] = { 4 };  

		fillPoly( mask,  
			ppt,  
			npt,  
			1,  
			Scalar( 255 ),  
			1 );    
		//imshow("mask", mask);
		return true;
	}else
	{
		return false;
	}
}

void saveData(const char filename[]){
	ofstream o_file;
	o_file.open(filename);
	int num =0 ;
	for (int i=0;i<allPoints.size();i++)
	{
		for (int j=0;j<allPoints[i].size();j++)
		{
			o_file<<allPoints[i][j].x<<","<<allPoints[i][j].y<<endl;
			num++;
		}
	}

	o_file.close();
	cout<<"totalFrameNumber "<<totalFrameNumber<<endl;
	cout<<"allPoints.size "<<allPoints.size()<<endl;
	cout<<"total points "<<num<<endl;
	printf("Data saved!");
}