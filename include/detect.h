#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/highgui.hpp>
#include "opencv2/aruco.hpp" 
#include "opencv2/aruco/dictionary.hpp" 
#include "opencv2/aruco/charuco.hpp" 
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;



//�ǵ��ID�����
std::vector<std::vector<cv::Point2f> > markerCorners;//ÿ����ά���4���ǵ�
std::vector<int> markerIds;//ÿ����ά���ID
uint32_t marker_area[10];//10����ά������
Point centerpoint[10];//��ౣ��10����ά������ĵ�
uint32_t area_sort[10] = { 0,1,2,3,4,5,6,7,8,9 };//�洢��ά������Ӵ�С�����
//int detected_arucoID[11] = {0};//�����Ѽ����Ķ�ά���ID,��10��Ԫ�ر����˵�ǰ�Ѽ��Ķ�ά����Ŀ
std::vector<int> detected_arucoID;//�����Ѽ����Ķ�ά���ID,��10��Ԫ�ر����˵�ǰ�Ѽ��Ķ�ά����Ŀ
//��������
void area(std::vector< std::vector<Point2f> >& markerCorners, std::vector<int>& markerIds);
//void detect_aruco_marker(Mat& markerImage, std::vector< std::vector<Point2f> >& markerCorners, std::vector<int>& markerIds);
int detect_aruco_marker(Mat& markerImage, std::vector< std::vector<Point2f> >& markerCorners, std::vector<int>& markerIds);
Point calcenterpoint(std::vector<Point2f> & markerCorner);//�����ά�����ĵ�
void insertSort(uint32_t *marker_area, uint32_t* a, int n);//�������С�Ӵ�С���򣬣�������±꣩


//#define DEBUG
/*
* ֱ�Ӳ�������,�Ӵ�С����
*
* ����˵����
marker_area -- ���
*     a -- �����������
*     n -- ����ĳ���
*/
void insertSort(uint32_t *marker_area, uint32_t* a, int n)
{
	int i, j, k;

	for (i = 1; i < n; i++)
	{
		//Ϊa[i]��ǰ���a[0...i-1]������������һ�����ʵ�λ��
		for (j = i - 1; j >= 0; j--)
			if (marker_area[a[j]] > marker_area[a[i]])
				break;

		//���ҵ���һ�����ʵ�λ��
		if (j != i - 1)
		{
			//����a[i]������������
			uint32_t temp = a[i];
			for (k = i - 1; k > j; k--)
				a[k + 1] = a[k];
			//��a[i]�ŵ���ȷλ����
			a[k + 1] = temp;
		}
	}
}
//int detect_aruco_marker(Mat& markerImage, std::vector< std::vector<Point2f> >& markerCorners, std::vector<int>& markerIds)
int detect_aruco_marker(Mat& markerImage, std::vector< std::vector<Point2f> >& markerCorners, std::vector<int>& markerIds)
{
	int right_marker = -1;
	Mat inputImage;
	markerImage.copyTo(inputImage);

	if (!inputImage.empty())
	{

#ifdef DEBUG
		namedWindow("ԭʼͼƬ", 0);
		imshow("ԭʼͼƬ", markerImage);
#endif
		////��ԭʼͼƬԤ����
		//for (int i = 0; i < inputImage.rows; i++)
		//{
		//	for (int j = 0; j < inputImage.cols; j++)
		//	{
		//		Point p_1;
		//		p_1.x = j;
		//		p_1.y = i;
		//		//if (abs(inputImage.at<Vec3b>(p_1)[0] - inputImage.at<Vec3b>(p_1)[1]) > 10 || abs(inputImage.at<Vec3b>(p_1)[0] - inputImage.at<Vec3b>(p_1)[2]) > 10 || abs(inputImage.at<Vec3b>(p_1)[1] - inputImage.at<Vec3b>(p_1)[2]) > 10)
		//		//{
		//		//	//�ټӸ�if����Ϊ�˽�����򿴶�ά��ʱ��Զ��������ɫ���ŵ����⡣
		//		//	if (inputImage.at<Vec3b>(p_1)[0] > 30 || inputImage.at<Vec3b>(p_1)[1] > 30 || inputImage.at<Vec3b>(p_1)[2] > 30)
		//		//	{
		//		//		inputImage.at<Vec3b>(p_1)[0] = 255;
		//		//		inputImage.at<Vec3b>(p_1)[1] = 255;
		//		//		inputImage.at<Vec3b>(p_1)[2] = 255;
		//		//	}

		//		//	
		//		//}
		//		
		//		if (abs(inputImage.at<Vec3b>(p_1)[0] - inputImage.at<Vec3b>(p_1)[1]) < 5 && abs(inputImage.at<Vec3b>(p_1)[0] - inputImage.at<Vec3b>(p_1)[2]) < 5 && abs(inputImage.at<Vec3b>(p_1)[1] - inputImage.at<Vec3b>(p_1)[2]) < 5)
		//		{
		//			//�ټӸ�if����Ϊ�˽�����򿴶�ά��ʱ��Զ��������ɫ���ŵ����⡣
		//			if ((5 < inputImage.at<Vec3b>(p_1)[0] && inputImage.at<Vec3b>(p_1)[0] < 150) && (5 < inputImage.at<Vec3b>(p_1)[1] && inputImage.at<Vec3b>(p_1)[1] < 150) && (5 < inputImage.at<Vec3b>(p_1)[2] && inputImage.at<Vec3b>(p_1)[2] < 150))
		//			{
		//				inputImage.at<Vec3b>(p_1)[0] = 0;
		//				inputImage.at<Vec3b>(p_1)[1] = 0;
		//				inputImage.at<Vec3b>(p_1)[2] = 0;
		//			}
		//			else
		//			{
		//				inputImage.at<Vec3b>(p_1)[0] = 255;
		//				inputImage.at<Vec3b>(p_1)[1] = 255;
		//				inputImage.at<Vec3b>(p_1)[2] = 255;
		//			}

		//		}
		//		else
		//		{
		//			inputImage.at<Vec3b>(p_1)[0] = 255;
		//			inputImage.at<Vec3b>(p_1)[1] = 255;
		//			inputImage.at<Vec3b>(p_1)[2] = 255;
		//		}
		//	}
		//}

		//imshow("Ԥ�����ͼƬ", inputImage);

		//cv::medianBlur(inputImage, inputImage, 3);//��ֵ�˲�
		//imshow("��ֵ�˲���ͼƬ", inputImage);
		//waitKey(1);
#ifdef DEBUG
		imshow("Ԥ�����ͼƬ", inputImage);
#endif


		//ʹ���ֵ�ΪDICT_ARUCO_ORIGINAL
		cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_ARUCO_ORIGINAL);
		//���
		cv::aruco::detectMarkers(inputImage, dictionary, markerCorners, markerIds);

		//������
		for (int i = 0; i < 10; i++)
			marker_area[i] = 0;

		int right = 0;
		for (int i = 0; i < markerIds.size(); i++)
		{

			//�������
			marker_area[i] = contourArea(markerCorners[i], false);

			//�������ĵ�
			centerpoint[i] = calcenterpoint(markerCorners[i]);//��ౣ��10����ά������ĵ�
			//����������һ��ֵ����ӡ�ǵ㣬����������߿�
			if (marker_area[i] > 50)
			{
				//cout << "corners: " << markerCorners.at(i) << endl;
				//cout << "Area: " << marker_area[i] << endl;
				cv::line(markerImage, markerCorners.at(i).at(0), markerCorners.at(i).at(1), Scalar(0, 0, 255), 3, CV_AA);
				cv::line(markerImage, markerCorners.at(i).at(1), markerCorners.at(i).at(2), Scalar(0, 0, 255), 3, CV_AA);
				cv::line(markerImage, markerCorners.at(i).at(2), markerCorners.at(i).at(3), Scalar(0, 0, 255), 3, CV_AA);
				cv::line(markerImage, markerCorners.at(i).at(3), markerCorners.at(i).at(0), Scalar(0, 0, 255), 3, CV_AA);

				// ��ʾ�����
				int font_face = cv::FONT_HERSHEY_PLAIN;//����
				double font_scale = 1.5;// �ߴ����ӣ�ֵԽ������Խ��  
				int thickness = 2;// ������� 
				cv::Point text_xy;
				text_xy = markerCorners.at(i).at(0);
				char str_temp[20];
				sprintf(str_temp, "%d", markerIds[i]);
				cv::putText(markerImage, str_temp, text_xy, font_face, font_scale, cv::Scalar(255, 0, 0), thickness, 8, 0);

				//����δ�����ģ����ұߵ�marker
				std::vector<int>::iterator ret;
				ret = std::find(detected_arucoID.begin(), detected_arucoID.end(), markerIds.at(i));
				if (ret == detected_arucoID.end())
				{
					if (right < markerCorners.at(i).at(3).x)
					{
						right = markerCorners.at(i).at(3).x;
						right_marker = i;
					}
				}
			}

		}
		//���������С >
		insertSort(marker_area, area_sort, 10);
		//for (int i = 0; i < markerIds.size(); i++)
		//{
		//	printf("marker_area[%d]:%d\n", area_sort[i], marker_area[area_sort[i]]);
		//}


#ifdef DEBUG
		//��inputImage�ϻ��������
		//cv::aruco::drawDetectedMarkers(inputImage, markerCorners, markerIds);
		imshow("detect", inputImage);
		waitKey(1);
#endif
	}

	return right_marker;//�������ұ�mark�����
}

void area(std::vector< std::vector<Point2f> >& markerCorners, std::vector<int>& markerIds)
{
	int height=100;
	for (int i = 0; i < markerIds.size(); i++)
	{
		if (markerCorners.at(i).at(0).x == markerCorners.at(i).at(1).x)
		{
			height = abs(markerCorners.at(i).at(0).y - markerCorners.at(i).at(1).y);
		}
		else if (markerCorners.at(i).at(0).x == markerCorners.at(i).at(2).x)
		{
			height = abs(markerCorners.at(i).at(0).y - markerCorners.at(i).at(2).y);
		}
		else if (markerCorners.at(i).at(0).x == markerCorners.at(i).at(3).x)
		{
			height = abs(markerCorners.at(i).at(0).y - markerCorners.at(i).at(3).y);
		}
		else if (markerCorners.at(i).at(1).x == markerCorners.at(i).at(2).x)
		{
			height = abs(markerCorners.at(i).at(1).y - markerCorners.at(i).at(2).y);
		}
		else if (markerCorners.at(i).at(1).x == markerCorners.at(i).at(3).x)
		{
			height = abs(markerCorners.at(i).at(1).y - markerCorners.at(i).at(3).y);
		}
		else if (markerCorners.at(i).at(2).x == markerCorners.at(i).at(3).x)
		{
			height = abs(markerCorners.at(i).at(2).y - markerCorners.at(i).at(3).y);
		}
		else
		{
			height = 100;
		}
		//marker_area[i] = contourArea(markerCorners[i], true);
		marker_area[i] = height * height;
	}
	return;

}


Point calcenterpoint(std::vector<Point2f>& markerCorner)
{
	Point centerpoint;
	centerpoint.x = (markerCorner[0].x + markerCorner[1].x + markerCorner[2].x + markerCorner[3].x) / 4.0 + 0.5;
	centerpoint.y = (markerCorner[0].y + markerCorner[1].y + markerCorner[2].y + markerCorner[3].y) / 4.0 + 0.5;
	return centerpoint;
}

//Point2f calcenterpoint(vector< vector<Point2f> >& markerCorners, int p)
//{
//	float centerx = 0.0;
//	float centery=0.0;
//	if (markerCorners.at(p).at(0).y == markerCorners.at(p).at(1).y)
//	{
//		centerx = (markerCorners.at(p).at(0).x + markerCorners.at(p).at(1).x) / 2.0;
//	}
//	else if (markerCorners.at(p).at(0).y == markerCorners.at(p).at(2).y)
//	{
//		centerx = (markerCorners.at(p).at(0).x + markerCorners.at(p).at(2).x) / 2.0;
//	}
//	else if (markerCorners.at(p).at(0).y == markerCorners.at(p).at(3).y)
//	{
//		centerx = (markerCorners.at(p).at(0).x + markerCorners.at(p).at(3).x) / 2.0;
//	}
//	else if (markerCorners.at(p).at(1).y == markerCorners.at(p).at(2).y)
//	{
//		centerx = (markerCorners.at(p).at(1).x + markerCorners.at(p).at(2).x) / 2.0;
//	}
//	else if (markerCorners.at(p).at(1).y == markerCorners.at(p).at(3).y)
//	{
//		centerx = (markerCorners.at(p).at(1).x + markerCorners.at(p).at(3).x) / 2.0;
//	}
//	else if (markerCorners.at(p).at(2).y == markerCorners.at(p).at(3).y)
//	{
//		centerx = (markerCorners.at(p).at(2).x + markerCorners.at(p).at(3).x) / 2.0;
//	}
//	else
//	{
//		centerx = (markerCorners.at(p).at(2).x + markerCorners.at(p).at(1).x) / 2.0;
//	}
//	centerpoint.x = centerx;
//	//centerpoint[0] = centerx;
//	//cout << "centerx: " << centerx << endl;
//	//cout << "centerpoint[0]: " << centerpoint[0] << endl;
//	if (markerCorners.at(p).at(0).x == markerCorners.at(p).at(1).x)
//	{
//		centery = (markerCorners.at(p).at(0).y + markerCorners.at(p).at(1).y)/2.0;
//	}
//	else if (markerCorners.at(p).at(0).x == markerCorners.at(p).at(2).x)
//	{
//		centery = (markerCorners.at(p).at(0).y + markerCorners.at(p).at(2).y)/2.0;
//	}
//	else if (markerCorners.at(p).at(0).x == markerCorners.at(p).at(3).x)
//	{
//		centery =(markerCorners.at(p).at(0).y + markerCorners.at(p).at(3).y)/2.0;
//	}
//	else if (markerCorners.at(p).at(1).x == markerCorners.at(p).at(2).x)
//	{
//		centery = (markerCorners.at(p).at(1).y + markerCorners.at(p).at(2).y) / 2.0;
//	}
//	else if (markerCorners.at(p).at(1).x == markerCorners.at(p).at(3).x)
//	{
//		centery = (markerCorners.at(p).at(1).y + markerCorners.at(p).at(3).y) / 2.0;
//	}
//	else if (markerCorners.at(p).at(2).x == markerCorners.at(p).at(3).x)
//	{
//		centery = (markerCorners.at(p).at(2).y + markerCorners.at(p).at(3).y) / 2.0;
//	}
//	else
//	{
//		centery = (markerCorners.at(p).at(2).y + markerCorners.at(p).at(1).y) / 2.0;
//	}
//	centerpoint.y = centery;
//	//centerpoint[1] = centery;
//	//cout << "centery: " << centery << endl;
//	return centerpoint;
//}