//airsim
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "api/RpcLibClientBase.hpp"
//opencv
#include "opencv2/opencv.hpp"

#include <iostream>
#include "WINDOWS.h"

#include "timer.h" 
#include <thread>
#include "getImg.h" //���ͼ����


//�����ռ�
using namespace std;
using namespace cv;
using namespace msr::airlib;

typedef ImageCaptureBase::ImageRequest ImageRequest;	//ͼ������
typedef ImageCaptureBase::ImageResponse ImageResponse;	//ͼ����Ӧ
typedef ImageCaptureBase::ImageType ImageType;			//ͼ������

// V ������� ****************************************
//airsim ���
msr::airlib::MultirotorRpcLibClient client("localhost", 41451, 60000);//����localhost:41451 	
//����������
//GpsData			 GPS_data;			//GPS
BarometerData	 Barometer_data;	//��ѹ��
MagnetometerData Magnetometer_data;	//������
ImuData			 Imu_data;			//IMU
static std::mutex g_mutex_img;//������
static std::mutex g_mutex_sensor;//���������ݻ�������������ѹ�ơ������ơ�IMU����
int avoid_obstacle_flag = 0; //���ϱ�־

int flag_mode = 1;//  1��ʾ����1��Ȧ��2��ʾ����2С����



// A ������� ****************************************//



// V ������ ****************************************

// A ������ ****************************************//


//�߳�
//�����ɵȺ�ʱ��
HANDLE hTimer_Key_Scan = CreateWaitableTimer(NULL, FALSE, NULL);
HANDLE hTimer_get_img = CreateWaitableTimer(NULL, FALSE, NULL);
HANDLE hTimer_get_sensor = CreateWaitableTimer(NULL, FALSE, NULL);
HANDLE hTimer_move_control = CreateWaitableTimer(NULL, FALSE, NULL);

void Key_Scan(void);
void get_img(void);
void get_sensor(void);//��ȡ����������
//����
volatile int key_value_cv = -1;
static int key_control(int key);//��������

//ͼ��
cv::Mat front_image, down_image, depth_image;

//ȫ�ֱ�־λ
bool flag_exit = 0;//���δtrue���ʾ�Ƴ�����

int main()
{
	while (RpcLibClientBase::ConnectionState::Connected != client.getConnectionState())
		client.confirmConnection();//���ӳ���

	while (!client.isApiControlEnabled())
		client.enableApiControl(true);//��ȡapi����

	client.armDisarm(true);//�����ɿ�
						  
	client.hover();//hoverģʽ
	
	
	//���ö�ʱ��ʱ��
	INT64 nDueTime = -0 * _SECOND;//��ʱ����Чʱ�䣬����
	SetWaitableTimer(hTimer_Key_Scan, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	SetWaitableTimer(hTimer_get_img, (PLARGE_INTEGER)&nDueTime, 400, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	SetWaitableTimer(hTimer_get_sensor, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	SetWaitableTimer(hTimer_move_control, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms
	
	printf("�̳߳�ʼ��\n");
	std::thread t_Key_Scan(Key_Scan);
	std::thread t_get_img(get_img);
	std::thread t_get_sensor(get_sensor);
	std::thread t_move_control(move_control);

	printf("�̳߳�ʼ�����\n");
	t_Key_Scan.join(); //�������ȴ����߳��˳�
	t_get_img.join();
	t_get_sensor.join();
	t_move_control.join();
	//�رն�ʱ��
	CloseHandle(hTimer_Key_Scan);
	CloseHandle(hTimer_get_img);
	CloseHandle(hTimer_get_sensor);
	CloseHandle(hTimer_move_control);
	
	//�ݻ�����OpenCV����
	cv::destroyAllWindows();
	
	printf("�����߳��˳����������\n");
	return 0;
}


void Key_Scan(void)
{
	clock_t time_1 = clock();//get time
	
	while (true)
	{
		//printf("Key_Scan��ʱ:%d ms\n", clock() - time_1);
		time_1 = clock();
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_Key_Scan, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		//��ʾͼ��
		g_mutex_img.lock();//�����
		if (!front_image.empty())
		{
			cv::imshow("FROWARD", front_image);
		}
		if (!down_image.empty())
		{
			cv::imshow("DOWN", down_image);
		}
		if (!depth_image.empty())
		{
			cv::imshow("FROWARD_DEPTH", depth_image);
		}
		g_mutex_img.unlock();//�ͷ���
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
		key_control(key_value_cv);//ִ�а�������
	
	}
	
}

void get_img(void)
{
	int i = 0;
	clock_t time_1= clock();//get time

	std::vector<ImageRequest> request = { ImageRequest(0, ImageType::Scene) , ImageRequest(0, ImageType::DepthPerspective, true), ImageRequest(3, ImageType::Scene) };
	while (true)
	{

		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_get_img, INFINITE);

		time_1 = clock();
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		std::vector<ImageResponse>& response = client.simGetImages(request);//��ȡͼ��
		if (response.size() > 0)
		{
			g_mutex_img.lock();//�����
			front_image = cv::imdecode(response.at(0).image_data_uint8, cv::IMREAD_COLOR);	//;ǰ��ͼ
			down_image = cv::imdecode(response.at(2).image_data_uint8, cv::IMREAD_COLOR);	//����ͼ	
			//���ͼ
			depth_image = Mat(response.at(1).height, response.at(1).width, CV_16UC1);
			// ת�����ͼ��ø����������ͼ
			avoid_obstacle_flag = imageResponse2Mat(response.at(1), depth_image);//����ֵΪ����

			g_mutex_img.unlock();//�ͷ���
		}

		//printf("    get_img��ʱ:%d ms\n", clock() - time_1);





		switch (flag_mode)//����1������2
		{
		default:
		case 1://����1 ��Ȧ
			
			break;
		case 2://����2 С����
			
			break;
		}




	}
	
}

void move_control(void)//�ƶ������߳�
{
	clock_t time_1= clock();//get time
	

	while (true)
	{
		float roll = 0.0f;//��x����ʱ�� //��λ�ǻ���
		float pitch = 0.0f;//��y����ʱ��  
		float yaw = 0.0f; //��z����ʱ��
		float duration = 0.05f;//����ʱ��
		float throttle = 0.587402f;//�պõ�������ʱ������
		float yaw_rate = 0.0f;

		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_move_control, INFINITE);
	
	
	}


}
static int key_control(int key)//��������
{
	clock_t time_1;// = clock();//get time
	//float roll = 0.1f;//��x����ʱ�� //��λ�ǻ���
	//float pitch = 0.1f;//��y����ʱ��  
	//float yaw = 0.0f; //��z����ʱ��
	//float duration = 0.05f;//����ʱ��
	//float throttle = 0.587402f;//�պõ�������ʱ������
	//float yaw_rate = 0.1f;

	float roll = 0.0f;//��x����ʱ�� //��λ�ǻ���
	float pitch = 0.0f;//��y����ʱ��  
	float yaw = 0.0f; //��z����ʱ��
	float duration = 0.05f;//����ʱ��
	float throttle = 0.587402f;//�պõ�������ʱ������
	float yaw_rate = 0.0f;

	bool flag = false;//Ϊ��ʱִ�п��ƺ���
	switch (key)
	{
	case 27://ESC
		flag_exit = true;//�˳���־λ��λ
		break;
	case 32://�ո�
		client.hover();//hoverģʽ
		printf("hover\n");
		flag = true;
		break;
	case 'w':
		flag = true;
		throttle += 0.1f;
		break;
	case 's':
		flag = true;
		throttle -= 0.1f;
		break;
	case 'a'://��תʱ���½�...
		flag = true;
		yaw_rate = -0.7f;
		break;
	case 'd':
		flag = true;
		yaw_rate = 0.7f;
		break;

	//�������Ի�ͷ����ǰ������
	case 'i'://pitch y����ʱ��Ƕ�
		flag = true;
		pitch = -0.1f;//��y����ʱ��
		throttle = throttle/cos(pitch)/cos(roll);//�պõ�������ʱ������
		break;
	case 'k'://pitch y����ʱ��Ƕ�
		flag = true;
		pitch = 0.1f;//��y����ʱ��
		throttle = throttle / cos(pitch) / cos(roll);//�պõ�������ʱ������

		break;
	case 'j'://roll x����ʱ��Ƕ�
		flag = true;
		roll = -0.1f;//��y����ʱ��
		throttle = throttle / cos(pitch) / cos(roll);//�պõ�������ʱ������

		break;
	case 'l'://roll x����ʱ��Ƕ�
		flag = true;
		roll = 0.1;//��y����ʱ��
		throttle = throttle / cos(pitch) / cos(roll);//�պõ�������ʱ������
		break;
	case 2490368://�������
		throttle += 0.0003f;
		printf("throttle=%f\n", throttle);
		break;
	case 2621440://�������

		throttle -= 0.0003f;
		printf("throttle=%f\n", throttle);

		break;
	default:
		break;
	}
	if (flag)
	{
		flag = false;
		client.moveByAngleThrottle(pitch, roll, throttle, yaw_rate, duration);
	}
	
	key_value_cv = -1;
	return 0;
}

void get_sensor(void)//��ȡ����������
{
	clock_t time_1 = clock();
	while (true)
	{
		//printf("  get_sensor��ʱ:%d ms\n", clock() - time_1);
		time_1 = clock();
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer_get_sensor, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		//���´���������
		g_mutex_sensor.lock();//������������
		Barometer_data = client.getBarometerdata();
		Magnetometer_data = client.getMagnetometerdata();
		Imu_data = client.getImudata();
		g_mutex_sensor.unlock();//�������������ͷ�	
	}
}






