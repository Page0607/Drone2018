//airsim
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "api/RpcLibClientBase.hpp"

#include "opencv2/opencv.hpp"
#include <iostream>
#include "WINDOWS.h"
#include "timer.h" 
#include "pthread.h"
//#include "squares-by.hpp"

#include "PIDController.h"
#include "PID.h"

#include "yolo_v2_class.hpp"

#include "wgs_conversions/wgs_conversions.h"

//��Ȧ�������ӵ�ͷ�ļ�
//#include "saveImage.h"
#include "getImg.h"
//���aruco��ά��
#include "detect.h"
//�����ռ�
//using namespace std;
using namespace cv;
using namespace msr::airlib;

/*
���ID 0��4�ֱ��Ӧ������ǰ������ǰ������ǰ���������·�������󷽡�
����ʱֻ���õײ�����ͷ����ͼ��ǰ������ͷ�ĳ���ͼ�����ͼ��
*/
//�������ID
#define CAMERA_FRONT 0
#define CAMERA_FRONT_LEFT 1
#define CAMERA_FRONT_RIGHT 2
#define CAMERA_BELOW 3
#define CAMERA_BEHIND 4


//airsim ���
typedef ImageCaptureBase::ImageRequest ImageRequest;
typedef ImageCaptureBase::ImageResponse ImageResponse;
typedef ImageCaptureBase::ImageType ImageType;
msr::airlib::MultirotorRpcLibClient client("localhost", 41451, 60000);//����localhost:41451 
																	  /* ���is_rate = true����yaw_or_rate������Ϊ�Զ�/��Ϊ��λ�Ľ��ٶȣ�
																	  ����ζ����ϣ���������ƶ�ʱ�Ըý��ٶ���������������ת��
																	  ���is_rate = false����yaw_or_rate������Ϊ�����Ƕȣ�
																	  ����ζ����ϣ��������ת���ض��Ƕȣ���ƫ���������ƶ�ʱ���ָýǶȡ�
																	  */
YawMode yaw_mode(true, 0);
//DrivetrainType driveTrain = DrivetrainType::ForwardOnly;
DrivetrainType driveTrain = DrivetrainType::MaxDegreeOfFreedom;

float roll = 0.1f, roll_temp;//��x����ʱ�� //��λ�ǻ���
float pitch = 0.1f, pitch_temp;//��y����ʱ��  
float yaw = 0.0f; //��z����ʱ��
float duration = 0.2f;//����ʱ��
float throttle = 0.575f;
float yaw_rate = 0.1f;
float altitude_last;//��һ�ε�GPS�߶ȣ�������������
float altitude_cur;//��ǰ��GPS�߶ȣ�������������

//����������
GpsData			 GPS_data;
BarometerData	 Barometer_data;
MagnetometerData Magnetometer_data;
ImuData			 Imu_data;
//����GPS����������ϵתΪ��������ϵ y��Ӧlatitude��x��Ӧlongitude
// body_x = xcos + ysin
// body_y = ycos - xsin
double body_x = GPS_data.longitude * cos(0) + GPS_data.latitude * sin(0);
double body_y = GPS_data.latitude * cos(0) - GPS_data.longitude * sin(0);
double longitude_temp = (long)(GPS_data.longitude * 1e6) % 1000000;//����С�����6λ��
double latitude_temp = (long)(GPS_data.latitude * 1e6) % 1000000;//����С�����6λ��



//�߳�
DWORD WINAPI Key_Scan(LPVOID pVoid);//__stdcall �����ڷ��ص�������֮ǰ��������ջ��ɾ��
DWORD WINAPI get_img(LPVOID pVoid);
DWORD WINAPI Control_Z_Thread(LPVOID pVoid);//��������߶�
HANDLE hTimer1;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;//������									  
//int pthread_mutex_destroy(pthread_mutex_t *mutex1); //������������

//����
volatile int key_value_cv = -1;
static int key_control(int key);//��������

//ͼ��
std::vector<ImageRequest> request = { ImageRequest(0, ImageType::Scene), ImageRequest(0, ImageType::DepthPerspective, true), ImageRequest(3, ImageType::Scene) };
std::vector<ImageResponse> response;
cv::Mat rgb_image_front;	//ǰ�ӳ���ͼ
cv::Mat rgb_image_front_show;
cv::Mat depth_image_front;	//ǰ�����ͼ
cv::Mat rgb_image_down;		//���ӽǳ���ͼ
std::string save = "D:\\My Documents\\AirSim\\my_Drone\\" + std::string("rgb_image.jpg");//ͼ�񱣴�·��
std::vector<std::vector<Point>> squares;
Point squares_centre;//��⵽��ͣ�������������ͼ���е�����

//std::vector<ImageRequest> request = { ImageRequest(CAMERA_BELOW, ImageType::Scene) };//������ȡ���·��ĳ���ͼ(Ĭ��ѹ��)������
//std::vector<ImageResponse> response;//��Ӧ
//cv::Mat rgb_image;
//PID
uint8_t Congtrol_Z_flag = 0;//1��ʾҪ���Ƹ߶�
uint8_t Congtrol_Centre_flag = 0;//1��ʾҪ����ͣ��������
double Control_Z = 14.3f;//���Ƶĸ߶�
double Control_Z_tolerance = 0.5f;// 0.01f;//���Ƹ߶ȵ�����
PID my_PID;
uint8_t Congtrol_dis_flag = 0;//1��ʾҪ����ǰ�����Ϊ3m
//yolo
int gpu_id = 0;//��gpu0
std::string cfg_filename = "F:\\VS_Project\\source\\repos\\getimgTest\\getimgTest\\yolov3-voc-0616.cfg";//����
std::string weight_filename = "F:\\VS_Project\\source\\repos\\getimgTest\\getimgTest\\yolov3-voc-0616_14500.weights";//byȨ��
//string pic_filename = "F:\\VS_Project\\source\\repos\\yolo_cfg_weight_names\\number_pic\\10_81.jpg";//�����ͼƬ
//string name_filename = "F:\\VS_Project\\source\\repos\\yolo_cfg_weight_names\\yolo-obj.names";//������Ŷ�Ӧ�����
//Detector my_yolo(cfg_filename, weight_filename, gpu_id);//��ʼ�������
//Mat srcImage = imread(pic_filename);//��ȡͼƬ
//vector<bbox_t> box = my_yolo.detect(srcImage);//yolo���ͼƬ

//Detector detector_number(cfg_filename, weight_filename, gpu_id);//��ʼ�������
//Detector detector_number("yolo-voc.cfg", "yolo-voc.weights");
std::vector<bbox_t> result_vec_front;	//ǰ��ͼ�ļ����
std::vector<bbox_t> result_vec_down;	//����ͼ�ļ����

//��ת180��
void turn180(double reference);
double angle_cur;//atan2(y,x)=atan2(y/x)
//ȥָ����gps��
void goto_gps(double latitude, double longitude, double tolerance); //latitude��longitudeΪҪȥ�ľ�γ��
PID_GPS my_PID_GPS;//����GPS����
uint8_t Congtrol_GPS_flag = 0;//1��ʾ����GPS����

//����С��������
PID_TREE my_PID_TREE;//���ڶ�ά�����Ҷ���
int avoid_obstacle_flag = 0;
Point2f right_down(121.453281, 31.029951);
Point2f left_down(121.453264,31.030317);
Point2f left_up(121.453589, 31.030314 );
Point2f right_up(121.453582, 31.029956 );
//ÿ�����������
Point2d num0_Point(121.454678, 31.030126);//ͣ����0������
//Point2d start_point[] = { Point2d(121.453323,31.029982), Point2d(121.453323,31.030050), 
//						  Point2d(121.453256,31.030117), Point2d(121.453265,31.030211), 
//						  Point2d(121.453244,31.030270)};
//Point2d start_point[] = { Point2d(121.453323,31.029991), Point2d(121.453324,31.030086),
//						  Point2d(121.453261,31.030146), Point2d(121.453249,31.030208),
//						  Point2d(121.453267,31.030287) };
Point2d start_point[] = { Point2d(121.453329,31.030004), Point2d(121.453318,31.030078),
						  Point2d(121.453267,31.030165), Point2d(121.453270,31.030224),
						  Point2d(121.453267,31.030291) };
//Point2d start_point[] = { Point2d(121.453254,31.030276) };
uint32_t start_point_ord = 0;//��ʼ����ţ�����һ����ʼ������
//����С���������ĸ���õ�������ֱ��
#define f1(x) (-10.318182*x + 1284.210212)
#define f2(x) (-0.764706*x + 123.906550)
#define f3(x) (0.091703*x + 19.892723)
#define f4(x) (-9.622222*x + 1199.680314)
#define f5(x) (0.208000*x + 5.767641)
#define f6(x) (0.672131*x - 50.602664)

//����С���ֿ�����־ 1Ϊ��������

uint8_t Congtrol_search_flag = 0;
uint8_t sprint_flag = 0;//1Ϊ��̣������׮
bool Is_InArea(double latitude, double longitude)//��һ��GPS��latitude��longitude�ж��Ƿ���С���������ڡ����������ڷ���TRUE
{
	//for debug
	//printf("f1:%d\n", start_point[0].y > f1(start_point[0].x));
	//printf("f2:%d\n", start_point[1].y > f2(start_point[1].x));
	//printf("f3:%d\n", start_point[2].y > f3(start_point[2].x));
	//printf("f4:%d\n", start_point[3].y > f4(start_point[3].x));
	//printf("f5:%d\n", start_point[4].y > f5(start_point[4].x));
	//printf("f6:%d\n", start_point[5].y > f6(start_point[5].x));

	//printf("f1:%d\n", start_point[0].y < f1(start_point[0].x));
	//printf("f2:%d\n", start_point[1].y < f2(start_point[1].x));
	//printf("f3:%d\n", start_point[2].y < f3(start_point[2].x));
	//printf("f4:%d\n", start_point[3].y > f4(start_point[3].x));
	//printf("f5:%d\n", start_point[4].y > f5(start_point[4].x));
	//printf("f6:%d\n", start_point[5].y > f6(start_point[5].x));
	//latitude Ϊy
	if (latitude < f1(longitude))
		if (latitude < f2(longitude))
			if (latitude < f3(longitude)) 
				if (latitude > f4(longitude))
					if (latitude > f5(longitude))
						if (latitude > f6(longitude))
							return TRUE;

					
	return FALSE;
}
bool GetMarker = false;
bool moveToCentre(Point point1, Point point2);
void saveRGBImage(int id, ImageResponse& image_info_rgb);
void saveDepthImage(int id, ImageResponse& image_info_depth);
int main()
{
	//Initial = 0, Connected, Disconnected, Reset, Unknown
	while (RpcLibClientBase::ConnectionState::Connected != client.getConnectionState())
		client.confirmConnection();//���ӳ���
	while (!client.isApiControlEnabled())
		client.enableApiControl(true);//��ȡapi����
	client.armDisarm(true);//�����ɿ�
						   //client.takeoff(1.0f);//�����ɵȴ�1s
	client.hover();//hoverģʽ

	//��ʼ��PID����
	//PID_position_x.setParam();//λ�ÿ���PID
	//PID_position_y.setParam();//λ�ÿ���PID

	//���ȴ����ɵȺ�ʱ��
	hTimer1 = CreateWaitableTimer(NULL, FALSE, NULL);
	//���ö�ʱ��ʱ��
	INT64 nDueTime = -0 * _SECOND;//��ʱ����Чʱ�䣬����
	SetWaitableTimer(hTimer1, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
																				//DWORD nThreadID = 0;
	HANDLE hThread1 = CreateThread(NULL, 0, Key_Scan, NULL, 0, NULL);// &nThreadID);
	HANDLE hThread2 = CreateThread(NULL, 0, get_img, NULL, 0, NULL);// &nThreadID);
	HANDLE hThread3 = CreateThread(NULL, 0, Control_Z_Thread, NULL, 0, NULL);// &nThreadID);


	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(20));//


		if (-1 != key_value_cv)//����а�������
		{
			if (-1 == key_control(key_value_cv))
			{
				pthread_mutex_lock(&mutex1);//�����
				//�رն�ʱ��
				CloseHandle(hTimer1);
				Sleep(300);//�ر�ʱ�����������߳�������UE4ͨ�ţ���һ��
				//�ر��߳�
				CloseHandle(hThread1);
				CloseHandle(hThread2);
				CloseHandle(hThread3);
				//�ݻ�����OpenCV����
				cv::destroyAllWindows();

				return 0;
			}
		}

	}

	//�ȴ��̷߳���
	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);
	CloseHandle(hThread1);
	CloseHandle(hThread2);
	CloseHandle(hThread3);
	//�رն�ʱ��
	CloseHandle(hTimer1);
	return 0;
}


DWORD WINAPI Control_Z_Thread(LPVOID pVoid)
{
	int i = 0;
	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer1, INFINITE);
		if (++i >= 6)//50*6=300msִ��һ��
		{
			i = 0;
			//pthread_mutex_lock(&mutex1);//�����
			//�����Ҫ���Ƹ߶�
			if (1 == Congtrol_Z_flag)
			{
				////����GPS����
				//GPS_data = client.getGpsLocation();
				//���¸߶�����
				Barometer_data = client.getBarometerdata();
				//if(Control_Z - GPS_data.altitude > Control_Z_tolerance)
				//	client.moveByAngleThrottle(0.0f, 0.0f, 0.03 + 0.575, 0.0f, 0.05f);//����0.575�����ƽ��
				//else if (Control_Z - GPS_data.altitude < -Control_Z_tolerance)
				//	client.moveByAngleThrottle(0.0f, 0.0f, -0.03 + 0.575, 0.0f, 0.05f);//����0.575�����ƽ��

				//P ��������
				double Throttle_Z = my_PID.PIDZ(Control_Z, Control_Z_tolerance);//�趨�߶ȣ�����
				if (Throttle_Z > Control_Z_tolerance*0.1 || Throttle_Z < -Control_Z_tolerance*0.1)
				{
					//client.moveByAngleThrottle(0.0f, 0.0f, Throttle_Z + 0.575, 0.0f, 0.05f);//����0.6�����ƽ�⣬����50ms����������ͬ
					client.moveByAngleThrottle(0.0f, 0.0f, Throttle_Z + 0.575, 0.0f, 0.3f);//����0.6�����ƽ�⣬����50ms����������ͬ
				}

			}
			//pthread_mutex_unlock(&mutex1);//�ͷ���
		}
	}

}

DWORD WINAPI Key_Scan(LPVOID pVoid)
{
	clock_t time_1;// = clock();//get time
				   //namedWindow("get_img");

	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer1, INFINITE);

		pthread_mutex_lock(&mutex1);//�����

									//��ʾͼ��
		if (!rgb_image_front_show.empty())//��ʾǰ������ͷ�ĳ���ͼ
		{
			imshow("front_image_rgb", rgb_image_front_show);
		}
		if (!depth_image_front.empty())//��ʾǰ������ͷ�����ͼ
		{
			imshow("front_image_depth", depth_image_front);
		}
		if (!rgb_image_down.empty())//��ʾǰ������ͷ�����ͼ
		{
			imshow("down_image_rgb", rgb_image_down);
		}


		//����ɨ��
		if (-1 == key_value_cv)
		{

			key_value_cv = cv::waitKeyEx(1);
		}

		else
		{

			cv::waitKey(1);

		}


		while (-1 != cv::waitKey(1));//�ѻ���������󣬲Ż���ʾ1msͼ��
									 //std::cout << key_value_cv << std::endl;//��ʾ��ֵ
									 //if (-1 != key_value_cv)
									 //{
									 //	while (-1 != cv::waitKey(1));//�ѻ���������󣬲Ż���ʾ1msͼ��
									 //	if (-1 == key_control(key_value_cv))
									 //		break;
									 //}

		pthread_mutex_unlock(&mutex1);//�ͷ���
	}
	return 0;
}

DWORD WINAPI get_img(LPVOID pVoid)
{
	ofstream file("E:\\result.txt");
	int i = 0;
	clock_t time_1;// = clock();//get time
	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer1, INFINITE);

		pthread_mutex_lock(&mutex1);//�����
		if (++i >= 6)//50*6=300msִ��һ��
		{
			i = 0;
			//��ӡ�Ƿ���С����������
			//��õ�ǰGPS����
			GPS_data = client.getGpsLocation();
			//printf("Is_InArea:%d \n", Is_InArea(GPS_data.latitude, GPS_data.longitude));

			time_1 = clock();//get time
			//��ȡ����
			response = client.simGetImages(request);
			//printf("request time = %dms\n", clock() - time_1);
			if (response.size() > 0)
			{
				// -- down_rgb
				ImageResponse image_info_2 = response[2];
				rgb_image_down = cv::imdecode(image_info_2.image_data_uint8, cv::IMREAD_COLOR);

				
				// -- front_rgb
				ImageResponse image_info_0 = response[0];
				rgb_image_front = cv::imdecode(image_info_0.image_data_uint8, cv::IMREAD_COLOR);
				rgb_image_front.copyTo(rgb_image_front_show);
				//���aruco markers
				int right_marker = -1;
				right_marker = detect_aruco_marker(rgb_image_front_show, markerCorners, markerIds);
				//������ͨ������Ҫ���� ���aruco markers ����
				circle(rgb_image_front_show, Point(640 / 2, 480 / 2), 4, Scalar(0, 255, 0), -1);//����ͼ������
				rectangle(rgb_image_front_show, Point(300, 230), Point(340, 250), Scalar(255, 0, 0), 2, CV_AA);
				
				// -- front_depth
				ImageResponse image_info_1 = response[1];
				depth_image_front = Mat(image_info_1.height, image_info_1.width, CV_16UC1);
				// ת�����ͼ��ø����������ͼ
				avoid_obstacle_flag = imageResponse2Mat(image_info_1, depth_image_front);//����ֵΪ����

				switch (Congtrol_search_flag)
				{
				case 0:break;
				case 1:
					if (start_point_ord < sizeof(start_point) / sizeof(Point2d))
					{
						my_PID.PIDReset();//��λ����
						my_PID_GPS.PIDReset();//��λ����
						my_PID_TREE.PIDReset();//��λ����
						//Sleep(2000);//�ȴ���������
						//����18.3
						Control_Z = 18.3;
						//Control_Z = 33.3;
						Congtrol_Z_flag = 1;//��������
						printf("����%f\n", Control_Z);
						do//�ȴ��������
						{
							Sleep(100);
							//���¸߶�����
							Barometer_data = client.getBarometerdata();
						} while (abs(Barometer_data.altitude - Control_Z) > Control_Z_tolerance);
						//Sleep(2000);//�ȴ���������
						//printf("ȥ���½�\n");
						//goto_gps(left_down.y, left_down.x, 0.2e-4);//ȥ���½�
						cout << "�ƶ�����һ����ʼ��:" << start_point[start_point_ord] << endl;
						goto_gps(start_point[start_point_ord].y, start_point[start_point_ord].x, 0.2e-4);//ȥ��һ����ʼ��
						Sleep(2000);//�ȴ���������
								   //����14.3
						printf("����14.3\n");
						Control_Z = 14.3;
						Congtrol_Z_flag = 1;//��������
						do//�ȴ��������
						{
							Sleep(100);
							//���¸߶�����
							Barometer_data = client.getBarometerdata();
						} while (abs(Barometer_data.altitude - Control_Z) > Control_Z_tolerance);
						Sleep(2000);//�ȴ���������
						goto_gps(start_point[start_point_ord].y, start_point[start_point_ord].x, 0.2e-4);//�ٿ���һ��GPS
						Sleep(2000);//�ȴ���������
						//��׼С����
						printf("��׼С����\n");
						turn180(-99.49);
						//printf("����ǰ����룬3m\n");

						start_point_ord++;//��һ����ʼ�����+1
						printf("��������״̬\n");
						Congtrol_search_flag = 4;//������һ��״̬������	
					}
					else
					{
						printf("���س�ʼ��\n");
						Congtrol_search_flag = 9;//���뷵��״̬,�ص���ʼ��
					}
					break;
				default:break;
				case 4://���ϲ���

					   //if����f2��Χ��̧���߶ȣ��ƶ�����һ��GPS�㣬���ߵ�һ����ʼ���ҵ���������ά�룬Ҳ����
					   //if (!Is_InArea(GPS_data.latitude, GPS_data.longitude))
					if (GPS_data.latitude > f1(GPS_data.longitude) || (1 == start_point_ord && detected_arucoID.size() >= 2) 
																   || (2 == start_point_ord && detected_arucoID.size() >= 4) 
																   || (3 == start_point_ord && detected_arucoID.size() >= 6)
																   || (4 == start_point_ord && detected_arucoID.size() >= 9))
					{
						my_PID.PIDReset();//��λ����
						my_PID_GPS.PIDReset();//��λ����
						my_PID_TREE.PIDReset();//��λ����
						if (start_point_ord < sizeof(start_point) / sizeof(Point2d))
						{
							//Congtrol_search_flag = 2;//������һ��״̬,����ǰ�����
							//if (1 == start_point_ord)//����ǵ�һ����ʼ�㣬����һ�����룬��ֹײ��
							//{
							//	Congtrol_Z_flag = 1;//�رն���
							//	printf("��һ����ʼ�㣬����һ�����룬��ֹײ��\n");
							//	std::this_thread::sleep_for(std::chrono::duration<double>(0.05));//��ʱ50ms�ȶ��߹ر�
							//	client.moveByAngleThrottle(0.0f, -0.2f, throttle, 0.0f, 3.0);
							//	std::this_thread::sleep_for(std::chrono::duration<double>(3.0));
							//}
							Sleep(2000);//�ȴ���������
							//����18.3
							Control_Z = 18.3;
							//Control_Z = 33.3;
							Congtrol_Z_flag = 1;//��������
							printf("����%f\n", Control_Z);
							do//�ȴ��������
							{
								Sleep(100);
								//���¸߶�����
								Barometer_data = client.getBarometerdata();
							} while (abs(Barometer_data.altitude - Control_Z) > Control_Z_tolerance);
							//Sleep(2000);//�ȴ���������
							printf("�ƶ�����һ����ʼ��[%d]:", start_point_ord);
							cout <<  start_point[start_point_ord] << endl;
							goto_gps(start_point[start_point_ord].y, start_point[start_point_ord].x, 0.2e-4);
							Sleep(2000);//�ȴ���������
									   //����14.3
							printf("����14.3\n");
							Control_Z = 14.3;
							Congtrol_Z_flag = 1;//��������
							do//�ȴ��������
							{
								Sleep(100);
								//���¸߶�����
								Barometer_data = client.getBarometerdata();
							} while (abs(Barometer_data.altitude - Control_Z) > Control_Z_tolerance);
							Sleep(2000);//�ȴ���������
							goto_gps(start_point[start_point_ord].y, start_point[start_point_ord].x, 0.2e-4);//�ٿ���һ��GPS
							Sleep(2000);//�ȴ���������
							//��׼С����
							printf("��׼С����\n");
							turn180(-99.49);
							//printf("����ǰ����룬3m\n");

							start_point_ord++;//��һ����ʼ�����+1
						}
						else
						{
							printf("���س�ʼ��\n");
							Congtrol_search_flag = 9;//���뷵��״̬,�ص���ʼ��
						}


					}
					else//������ϲ���
					{
						cout << "avoid_obstacle_flag: " << avoid_obstacle_flag << ":";
						switch (avoid_obstacle_flag)
						{
						case 0:
							cout << "Impassability��" << endl;
							client.moveByAngleThrottle(0.1f, 0.0f, throttle, 0.0f, 0.3f);
							;//�������������
							break;
						case 1:
							cout << "Forward" << endl;

							Congtrol_Z_flag = 1;//��������
							//�����⵽��ά��
							//if (markerIds.size() > 0)
							if (-1 != right_marker && centerpoint[right_marker].x > 100)//�����⵽��ά�벢��ƫ�ұ�
							{
								//��ȡ��ǰ����
								double distance_cur = depth_image_front.at<uint16_t>(centerpoint[right_marker].y, centerpoint[right_marker].x);
								circle(rgb_image_front_show, Point(centerpoint[right_marker].x, centerpoint[right_marker].y), 4, Scalar(0, 255, 0), -1);//����ͼ������
								printf("distance_cur:%fmm\n", distance_cur);//add by LB
								//�������4.9m��
								if (distance_cur > 4.9*10000)
								{
									//�ƶ�
									GetMarker = moveToCentre(markerCorners.at(right_marker).at(0), markerCorners.at(right_marker).at(2));
								}
								else
								{
									//�����ά���ͼ����ά��ID�����ͼ���ݣ�
									printf("�����ά����Ϣ\n");
									detected_arucoID.push_back(markerIds[right_marker]);//��ά��ID
									//��ӡ�ѱ���Ķ�ά��
									printf("�Ѽ��Ķ�ά����Ŀ:%d\n", detected_arucoID.size());
									for (int i = 0; i<detected_arucoID.size(); i++)
										printf("%d\n", detected_arucoID.at(i));
									;//��ά���ͼ
									;//���ͼ����
									saveRGBImage(markerIds.at(right_marker), response[0]);
									saveDepthImage(markerIds.at(right_marker), response[1]);
									if (file.is_open())
									{
										printf("���浽result\n");
										file << markerIds.at(right_marker) << " " << markerCorners.at(right_marker).at(0).x << " " << markerCorners.at(right_marker).at(0).y
											<< " " << markerCorners.at(right_marker).at(2).x << " " << markerCorners.at(right_marker).at(2).y << "\n";
									}
									else
									{
										printf("file û��\n");
									}

									if (detected_arucoID.size() >= 10)//������������ж�ά�룬���س�ʼ��
									{
										printf("���س�ʼ��\n");
										Congtrol_search_flag = 9;//���뷵��״̬,�ص���ʼ��
									}
									else
									{
										printf("��������״̬\n");
										Congtrol_search_flag = 4;
									}
								}
							}
							else//δ��⵽��ά��
							{
								//client.moveByAngleThrottle(-0.1f, 0.0f, throttle, 0.0f, 0.3f);
								client.moveByAngleThrottle(-0.2f, 0.0f, throttle, 0.0f, 0.3f);//С������ǰ
							}
						

							break;
						case -2:
							cout << "Move Left" << endl;
							client.moveByAngleThrottle(0.0f, -0.15f, throttle, 0.0f, 0.3f);
							//sprint_flag = 1;
							break;
						case 2:
							cout << "Move Right" << endl;
							client.moveByAngleThrottle(0.0f, 0.15f, throttle, 0.0f, 0.3f);
							//sprint_flag = 1;
							break;
						default:
							break;
						}
					}

					break;
				case 9:
					Congtrol_search_flag = 10;//��һ��״̬
					//����18.3
					Control_Z = 24.0;
					//Control_Z = 33.3;
					Congtrol_Z_flag = 1;//��������
					printf("����%f\n", Control_Z);
					do//�ȴ��������
					{
						Sleep(100);
						//���¸߶�����
						Barometer_data = client.getBarometerdata();
					} while (abs(Barometer_data.altitude - Control_Z) > Control_Z_tolerance);
					Sleep(2000);//�ȴ���������
					//printf("�ƶ�����һ����ʼ��\n");
					cout << "������ʼ��" << num0_Point << endl;
					goto_gps(num0_Point.y, num0_Point.x, 0.6e-4);
					Sleep(500);//�ȴ���������
							   //����14.3
					printf("����14.3\n");
					Control_Z = 14.3;
					Congtrol_Z_flag = 1;//��������
					do//�ȴ��������
					{
						Sleep(100);
						//���¸߶�����
						Barometer_data = client.getBarometerdata();
					} while (abs(Barometer_data.altitude - Control_Z) > Control_Z_tolerance);
					Sleep(2000);//�ȴ���������
								//��׼ͣ����
					;
					//����
					Congtrol_Z_flag = 0;//�رն���
					client.land(60);
					//std::this_thread::sleep_for(std::chrono::duration<double>(1));
					break;
				}
			}

		}

		
		pthread_mutex_unlock(&mutex1);//�ͷ���
		
	}
	file.close();
	return 0;
}

static int key_control(int key)//��������
{
	clock_t time_1;// = clock();//get time



	pthread_mutex_lock(&mutex1);//�����
	switch (key)
	{
	case 7340032://F1
		std::cout << "press 'F1' for help" << std::endl;
		std::cout << "press 'ESC' exit" << std::endl;
		std::cout << "press 'T' Drone takeoff" << std::endl;
		std::cout << "press 'W' Drone rise" << std::endl;
		std::cout << "press 'S' Drone descend" << std::endl;
		std::cout << "press Arrow keys: Drone forward,retreat,left,right" << std::endl;
		std::cout << "press 'I' or 'K' or 'J' or 'L': Drone forward,retreat,left,right" << std::endl;
		break;
	case 32://�ո�
		client.moveByAngleThrottle(pitch, roll, throttle, yaw_rate, duration);
		break;
	case 27://ESC
		std::cout << "disconnection" << std::endl;
		pthread_mutex_unlock(&mutex1);//�ͷ���
		return -1;
		break;
	case 't'://take off
		std::cout << "takeoff,wait 1s " << std::endl;
		client.takeoff(1.0f);//�����ɵȴ�1s
		std::this_thread::sleep_for(std::chrono::duration<double>(1));
		client.moveByAngleThrottle(-0.01f, 0.0f, throttle, 0.0f, 0.05f);//ִ��һ�κ�GPS�߶Ȼ��ɸ�����ָ��������λ
		std::cout << "takeoff OK" << std::endl;

		break;
	case 'b'://land
		std::cout << "land" << std::endl;
		client.land(5.0f);//�����ɵȴ�1s
		std::this_thread::sleep_for(std::chrono::duration<double>(5.0f));
		std::cout << "land OK" << std::endl;

		break;

	case 'w':
		client.moveByAngleThrottle(0.0f, 0.0f, 0.7f, 0.0f, 0.2f);

		//roll += 0.1f;//��x����ʱ�� //��λ�ǻ���
		//printf("\n pitch:%f, roll:%f, throttle:%f, yaw_rate:%f, duration:%f\n", pitch, roll, throttle, yaw_rate, duration);
		break;
	case 's':
		client.moveByAngleThrottle(0.0f, 0.0f, 0.5f, 0.0f, 0.2f);
		//roll -= 0.1f;//��x����ʱ�� //��λ�ǻ���
		//printf("\n pitch:%f, roll:%f, throttle:%f, yaw_rate:%f, duration:%f\n", pitch, roll, throttle, yaw_rate, duration);


		break;
	case 'a'://��תʱ���½�...
		client.moveByAngleThrottle(0.0f, 0.0f, throttle, -1.0f, 0.2f);
		//client.moveByAngleThrottle(0.0f, 0.0f, 0.0f, 0.0f, 0.2f);
		//pitch += 0.1f;//��y����ʱ��  
		//printf("\n pitch:%f, roll:%f, throttle:%f, yaw_rate:%f, duration:%f\n", pitch, roll, throttle, yaw_rate, duration);


		break;
	case 'd':
		client.moveByAngleThrottle(0.0f, 0.0f, throttle, 1.0f, 0.2f);

		break;

	case 2490368://�߶ȿ�������
		Control_Z += 1.0f;
		printf("Control_Z=%f\n", Control_Z);
		break;
	case 2621440:////�߶ȿ��Ƽ���
		Control_Z -= 1.0f;
		printf("Control_Z=%f\n", Control_Z);

		break;
	case 2424832://����Ʈ��-Y����


		break;
	case 2555904://����Ʈ ��Y����

		break;

		//�������Ի�ͷ����ǰ������
	case 'i'://pitch y����ʱ��Ƕ�
		client.moveByAngleThrottle(-0.1f, 0.0f, throttle, 0.0f, 0.2f);
		//duration += 0.1f;//����ʱ��
		//printf("\n pitch:%f, roll:%f, throttle:%f, yaw_rate:%f, duration:%f\n", pitch, roll, throttle, yaw_rate, duration);


		break;
	case 'k'://pitch y����ʱ��Ƕ�
		client.moveByAngleThrottle(0.1f, 0.0f, throttle, 0.0f, 0.2f);
		//duration -= 0.1f;//����ʱ��
		//printf("\n pitch:%f, roll:%f, throttle:%f, yaw_rate:%f, duration:%f\n", pitch, roll, throttle, yaw_rate, duration);



		break;
	case 'j'://roll x����ʱ��Ƕ�
		client.moveByAngleThrottle(0.0f, -0.1f, throttle, 0.0f, 0.2f);

		break;
	case 'l'://roll x����ʱ��Ƕ�
		client.moveByAngleThrottle(0.0f, 0.1f, throttle, 0.0f, 0.2f);

		break;
	case '1'://��ʾGPS,��ʱ

		time_1 = clock();//get time
		//�����ʱ��period������
		GPS_data = client.getGpsLocation();
		Barometer_data = client.getBarometerdata();
		Magnetometer_data = client.getMagnetometerdata();
		Imu_data = client.getImudata();

		std::cout << std::endl;
		std::cout << "GPS[latitude longitude altitude]:" << GPS_data.to_string()<< std::endl; ;//��ӡGPS
		std::cout << "Barometer_data:" << Barometer_data << std::endl;
		std::cout << "Magnetometer_data:" << Magnetometer_data.magnetic_field_body << std::endl;
		std::cout << "Imu_data.angular_velocity:" << Imu_data.angular_velocity << std::endl;
		std::cout << "Imu_data.linear_acceleration:" << Imu_data.linear_acceleration << std::endl;//xy?z,��λ��g���������ٶȣ���
		//std::cout << "Position[x y z]:" << client.getPosition() << std::endl; ;//��ӡ����
		//printf("time_delta:\r\n");
		//printf("%d\r\n%d\r\n%d\r\n%d\r\n", time_2 - time_1, time_3 - time_2, time_4 - time_3, time_5 - time_4);
		
		std::cout << "Magnetometer_data[xyz]:" << Magnetometer_data.magnetic_field_body[0] << Magnetometer_data.magnetic_field_body[1] << Magnetometer_data.magnetic_field_body[2] << std::endl;
		angle_cur = atan2(Magnetometer_data.magnetic_field_body[1], Magnetometer_data.magnetic_field_body[0]);
		std::cout << "�Ƕ�:" << 180.0f * angle_cur / M_PI << std::endl; ;//atan2(y,x)=atan2(y/x)
		
		//longitude_temp = (long)(GPS_data.longitude * 1e6) % 1000000 / 1e3;//����С�����6λ��
		//latitude_temp = (long)(GPS_data.latitude * 1e6) % 1000000 / 1e3;//����С�����6λ��
		longitude_temp = GPS_data.longitude;
		latitude_temp = GPS_data.latitude;
		/*body_x = GPS_data.longitude * cos(angle_cur) + GPS_data.latitude * sin(angle_cur);
		body_y = GPS_data.latitude * cos(angle_cur) - GPS_data.longitude * sin(angle_cur);*/
		body_x = longitude_temp * cos(angle_cur) + latitude_temp * sin(angle_cur);
		body_y = latitude_temp * cos(angle_cur) - longitude_temp * sin(angle_cur);
		printf("%f \n",cos(M_PI));
		printf("body_x:%f,body_y:%f \n", body_x, body_y);
		printf("time :%d ms\n", clock() - time_1);
		break;
	case '2'://�л����Ƹ߶�
		if (1 == Congtrol_Z_flag)
		{
			Congtrol_Z_flag = 0;

		}
		else
		{
			Congtrol_Z_flag = 1;
		}
		printf("Congtrol_Z_flag=%d\n", Congtrol_Z_flag);


		break;
	case '3'://�л�����ͣ��������

		if (1 == Congtrol_Centre_flag)
		{
			Congtrol_Centre_flag = 0;

		}
		else
		{
			Congtrol_Centre_flag = 1;
			my_PID.PIDReset();//��λ����
		}
		printf("Congtrol_Centre_flag=%d\n", Congtrol_Centre_flag);
		break;
	case '4'://����ͼ��

		std::cout << save << std::endl;
		//imwrite(save, rgb_image);



		break;
	case '5'://���Ե�ͷ180��
		//flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
		//turn180();

		break;
	case '6'://С��������
		printf("С��������\n");

		//��������
		Control_Z = 14.3f;//���Ƶĸ߶ȣ���ά���������߶�
		Congtrol_Z_flag = 1;
		//��ͷ180��
		//turn180();

		break;
	case '7'://�л�����ǰ�����
		if (1 == Congtrol_dis_flag)
		{
			Congtrol_dis_flag = 0;

		}
		else
		{
			Congtrol_dis_flag = 1;
		}
		printf("Congtrol_dis_flag=%d\n", Congtrol_dis_flag);

		break;
	case '8'://�л�����С���� debug
		if (0 != Congtrol_search_flag)
		{
			Congtrol_search_flag = 0;

		}
		else
		{
			my_PID.PIDReset();//��λ����
			my_PID_GPS.PIDReset();//��λ����
			my_PID_TREE.PIDReset();//��λ����
			//Congtrol_search_flag = 1;
			//for debuge
			Congtrol_search_flag = 4;//for debug
			start_point_ord = 1;//for debug
			
		}
		printf("Congtrol_search_flag=%d\n", Congtrol_search_flag);

		break;
	
	case '9'://�л�����С���� test
		if (0 != Congtrol_search_flag)
		{
			Congtrol_search_flag = 0;

		}
		else
		{
			my_PID.PIDReset();//��λ����
			my_PID_GPS.PIDReset();//��λ����
			my_PID_TREE.PIDReset();//��λ����
			Congtrol_search_flag = 1;
		}
		printf("Congtrol_search_flag=%d\n", Congtrol_search_flag);

		

		break;
	case '0'://����

		break;
	case 'z'://���Ե�ͷ
		//flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
		turn180(0.0);

		break;
	case 'x'://���Ե�ͷ
			 //flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
		turn180(80.51065);

		break;
	case 'c'://���Ե�ͷ
			 //flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
		turn180(-99.49);

		break;
	case 'v'://ȥָ����gps��
		//flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
		//goto_gps(31.030126, 121.454678);
		

		if (1 == Congtrol_GPS_flag)
		{
			Congtrol_GPS_flag = 0;

		}
		else
		{
			Congtrol_GPS_flag = 1;
			my_PID_GPS.PIDReset();//��λ����
			//turn180(0.0);//������ͷָ�򱱼�
		}
		printf("Congtrol_GPS_flag=%d\n", Congtrol_GPS_flag);
		
		goto_gps(31.030126, 121.454678, 0.6e-4);
		break;
	case 'n'://ȥָ����gps��
			 //flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
			 //goto_gps(31.030126, 121.454678);
		if (1 == Congtrol_GPS_flag)
		{
			Congtrol_GPS_flag = 0;

		}
		else
		{
			Congtrol_GPS_flag = 1;
			my_PID_GPS.PIDReset();//��λ����
								  //turn180(0.0);//������ͷָ�򱱼�
		}
		printf("Congtrol_GPS_flag=%d\n", Congtrol_GPS_flag);

		//goto_gps(31.029951, 121.453281);
		goto_gps(right_down.y, right_down.x, 0.2e-4);
		break;
	case 'm'://ȥָ����gps��
			 //flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
			 //goto_gps(31.030126, 121.454678);
		if (1 == Congtrol_GPS_flag)
		{
			Congtrol_GPS_flag = 0;

		}
		else
		{
			Congtrol_GPS_flag = 1;
			my_PID_GPS.PIDReset();//��λ����
								  //turn180(0.0);//������ͷָ�򱱼�
		}
		printf("Congtrol_GPS_flag=%d\n", Congtrol_GPS_flag);

		goto_gps(left_down.y, left_down.x, 0.2e-4);
		break;
	

	case '+'://
		kd += 0.000001;
		my_PID.PIDReset();//��λ����
		printf("kd=%f\n", kd);
		break;
	case '-'://
		kd += 0.000001;
		my_PID.PIDReset();//��λ����
		printf("kd=%f\n", kd);
		break;
	default:
		; break;

	}

	key_value_cv = -1;//

	pthread_mutex_unlock(&mutex1);//�ͷ���
	return 0;
}



void turn180(double reference) //flag 0 ��ʾ��׼0��  1��׼80.51065   2��׼-99.49
{
	double tolerance = 0.5;//����1.5�����
	double reference0 = 0.0;//�趨ֵ
	double reference1 = 80.51065;//�趨ֵ
	double reference2 = -99.49;//�趨ֵ
	double lf;
	double kp = 0.05;
	uint8_t time_temp = 0;//����������N���ڷ�Χ����Ϊ�Ѷ�׼
	printf("->:%f ing...\n", reference);
	while (true)
	{
		//�����ͷ�������������ϵ�Ƕ�
		Magnetometer_data = client.getMagnetometerdata();
		angle_cur = 180.0f * atan2(Magnetometer_data.magnetic_field_body[1], Magnetometer_data.magnetic_field_body[0]) / M_PI;//atan2(y,x)=atan2(y/x)


		if (angle_cur < reference || angle_cur >(reference + tolerance))
		{
			if (time_temp != 0)
				kp = 0.0001;
			else
				kp = 0.02;
			time_temp = 0;
			if (angle_cur < reference) {
				lf = kp * (reference - angle_cur);
			}
			else if (angle_cur > reference + tolerance) {
				lf = kp * (reference + tolerance - angle_cur);
			}
			else {
				lf = 0;
			}
			lf = -lf;
			CLIP3(-0.5f, lf, 0.5f);
			client.moveByAngleThrottle(0.0f, 0.0f, throttle, lf, 0.05f);
			std::this_thread::sleep_for(std::chrono::duration<double>(0.05f));
		}
		else
		{
			time_temp++;
			printf("time_temp:%d\n", time_temp);
			if (time_temp > 2)
			{
				printf("->:%f OK!\n", reference);
				client.moveByAngleThrottle(0.0f, 0.0f, throttle, 0.0, 0.1f);//ֹͣת��
				std::this_thread::sleep_for(std::chrono::duration<double>(0.1f));
				break;
			}
			std::this_thread::sleep_for(std::chrono::duration<double>(0.05f));

		}

	}
}

void goto_gps(double latitude, double longitude, double tolerance) //latitude��longitudeΪҪȥ�ľ�γ��
{
	//double tolerance = 0.2e-4;//������� ����
	//double tolerance = 0.6e-4;//������� �ֵ�
	double angle_tolerance = 15.0;//���̽Ƕ����
	uint8_t flag = 0;//
	GpsData GPS_data_cur;

	turn180(0.0);//������ͷָ�򱱼�
	while (true)
	{
		//��ȡ��ǰ�Ƕ�ֵ
		Magnetometer_data = client.getMagnetometerdata();
		angle_cur = 180.0f * atan2(Magnetometer_data.magnetic_field_body[1], Magnetometer_data.magnetic_field_body[0]) / M_PI;
		//std::cout << "�Ƕ�:" << angle_cur << std::endl; ;//atan2(y,x)=atan2(y/x)
		if(angle_cur < 0.0-angle_tolerance || angle_cur > 0.0+angle_tolerance)
			turn180(0.0);//������ͷָ�򱱼�
		//��õ�ǰGPS����
		GPS_data_cur = client.getGpsLocation();
		//printf("gps_cur:%f,%f \n", GPS_data_cur.latitude, GPS_data_cur.longitude);
		
		flag = 0;
		double latitude_delta = latitude - GPS_data_cur.latitude;
		double longitude_delta = longitude - GPS_data_cur.longitude;
		pitch = my_PID_GPS.PIDX(latitude_delta, 0.15, tolerance);//0.436��25�㣬���ƽǶ����25�㣬�������0.2e-5
		roll = my_PID_GPS.PIDY(longitude_delta, 0.15, tolerance);//0.436��25�㣬���ƽǶ����25��
		printf("gps_err2:%lf,tolerance2:%lf \n", (latitude_delta*latitude_delta + longitude_delta*longitude_delta) * 1e8, tolerance * tolerance * 1e8);//��ӡ��ǰgps���
		if (tolerance * tolerance > (latitude_delta*latitude_delta + longitude_delta*longitude_delta))
		{
			GPS_data_cur = client.getGpsLocation();
			printf("goto_gps:%lf,%lf OK!,gps_err2:%lf,tolerance2:%lf\n", latitude, longitude, (latitude_delta*latitude_delta + longitude_delta*longitude_delta) * 1e8, tolerance * tolerance * 1e8);
			
			break;
		}
		client.moveByAngleThrottle(pitch, -roll, throttle, 0.0f, 0.250);//
		std::this_thread::sleep_for(std::chrono::duration<double>(0.300));


	}
}
bool moveToCentre(Point point1, Point point2)
{
	//ImageResponse image_info = response[1];
	//int x0 = 280, y0 = 200, x1 = 360, y1 = 280;
	int pixel_safe[] = { 250 ,100};//��߰�ȫ����,�ұ߰�ȫ����
	//float depth_safe = 4;
	//pixel_safe = 0.5f / ((float)point2.y - (float)point1.y)*(float)1.0;
	//Point center;
	//center.x = (point2.x + point1.x) / 2;
	//center.y = (point2.y + point1.y) / 2;
	int move_direction;    //0:���ľ��뻭��������    1�������ϱ����   2�������ұ����   3�������±����
	int distance_min;
	int distance_toside[2] = { point1.x , 640 - point2.x };    //������ͼ���ĸ��ߵľ���

	distance_min = 2000;
	for (int i = 0; i< sizeof(distance_toside)/sizeof(int); i++)
	{
		if (distance_min>distance_toside[i])
		{
			distance_min = distance_toside[i];
			move_direction = i;
		}
	}
	if (distance_min < pixel_safe[move_direction])
	{
		switch (move_direction)
		{
		case 0:
			client.moveByAngleThrottle(0.0f, -0.2f, throttle, 0.0f, 0.3f);
			cout << "moveleft" << endl;
			break;
		case 1:
			client.moveByAngleThrottle(0.0f, 0.2f, throttle, 0.0f, 0.3f);
			cout << "moveright" << endl;
			break;
		}
	}
	else
	{
		client.moveByAngleThrottle(-0.25f, 0.0f, throttle, 0.0f, 0.3f);//С������ǰ
		cout << "movefront" << endl;
	}


	return false;
}

void saveRGBImage(int id, ImageResponse& image_info_rgb)
{
	char filename[100];
	sprintf_s(filename, "E:\\images\\%d.png", id);
	std::ofstream file(filename, std::ios::binary);
	file.write(reinterpret_cast<const char*>(image_info_rgb.image_data_uint8.data()), image_info_rgb.image_data_uint8.size());
	file.close();
	cout << filename << endl;
}

void saveDepthImage(int id, ImageResponse& image_info_depth)
{
	char filename[100];
	sprintf_s(filename, "E:\\depth\\%d.pfm", id);
	Utils::writePfmFile(image_info_depth.image_data_float.data(), image_info_depth.width, image_info_depth.height, filename);
	cout << filename << endl;
}
