//airsim
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "api/RpcLibClientBase.hpp"
//opencv
#include "opencv2/opencv.hpp"

#include <iostream>
#include "WINDOWS.h"

#include "timer.h" 
//#include <pthread.h>
#include <thread>

//�����ռ�
using namespace std;
using namespace cv;
using namespace msr::airlib;

//airsim ���
msr::airlib::MultirotorRpcLibClient client("localhost", 41451, 60000);//����localhost:41451 	
typedef ImageCaptureBase::ImageRequest ImageRequest;
typedef ImageCaptureBase::ImageResponse ImageResponse;
typedef ImageCaptureBase::ImageType ImageType;

//�߳�
HANDLE hTimer1;//��ʱ��
static std::mutex g_mutex;//������
void Key_Scan(void);
void get_img(void);
//����
volatile int key_value_cv = -1;
static int key_control(int key);//��������

//ͼ��
cv::Mat front_image, down_image,test_image;

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
	
	//���ȴ����ɵȺ�ʱ��
	hTimer1 = CreateWaitableTimer(NULL, FALSE, NULL);
	//���ö�ʱ��ʱ��
	INT64 nDueTime = -0 * _SECOND;//��ʱ����Чʱ�䣬����
	SetWaitableTimer(hTimer1, (PLARGE_INTEGER)&nDueTime, 50, NULL, NULL, FALSE);//50��ʾ��ʱ������50ms

	printf("�̳߳�ʼ��\n");
	std::thread t_Key_Scan(Key_Scan);
	std::thread t_get_img(get_img);
	
	printf("�̳߳�ʼ�����\n");
	//t_get_img.detach();
	//t_Key_Scan.detach();
	t_get_img.join();//�������ȴ����߳��˳�
	t_Key_Scan.join();
	//�رն�ʱ��
	CloseHandle(hTimer1);
	//�ݻ�����OpenCV����
	cv::destroyAllWindows();
	
	printf("�����߳��˳����������\n");
	return 0;
}

//
//DWORD WINAPI Control_Z_Thread(LPVOID pVoid)
//{
//	int i = 0;
//	while (true)
//	{
//		//�ȴ���ʱ��ʱ�䵽��
//		WaitForSingleObject(hTimer1, INFINITE);
//	}
//
//}

void Key_Scan(void)
{
	clock_t time_1;// = clock();//get time
	
	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer1, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}

		//��ʾͼ��
		g_mutex.lock();//��
		if (!front_image.empty())
		{
			cv::imshow("FROWARD", front_image);
		}
		if (!down_image.empty())
		{
			cv::imshow("DOWN", down_image);
		}
		g_mutex.unlock();//�ͷ���
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

		//g_mutex.unlock();//�ͷ���
		
		
	}
	
}

void get_img(void)
{
	int i = 0;
	clock_t time_1;// = clock();//get time

	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer1, INFINITE);
		if (flag_exit)//�˳��߳�
		{
			return;
		}
		//g_mutex.lock();//�����
		if (++i >= 6)//50*6=300msִ��һ��
		{
			i = 0;
			time_1 = clock();//get time

			std::vector<ImageRequest> request = { ImageRequest(0, ImageType::Scene) , ImageRequest(0, ImageType::DepthPerspective, true), ImageRequest(3, ImageType::Scene) };

			std::vector<ImageResponse>& response = client.simGetImages(request);

			if (response.size() > 0)
			{

				//�����Ǹı�ָ��ָ�򣬲���image copy�����Բ��ü���
				g_mutex.lock();//��
				front_image = cv::imdecode(response.at(0).image_data_uint8, cv::IMREAD_COLOR);	//;ǰ��ͼ
				down_image = cv::imdecode(response.at(2).image_data_uint8, cv::IMREAD_COLOR);	//����ͼ	
				g_mutex.unlock();//�ͷ���
			}
		}
		//g_mutex.unlock();//�ͷ���
	}
	
}

static int key_control(int key)//��������
{
	clock_t time_1;// = clock();//get time

	switch (key)
	{
	case 27://ESC
		flag_exit = true;//�˳���־λ��λ
		break;
	case 32://�ո�
		client.hover();//hoverģʽ
		printf("hover\n");
		break;
	case 'w':
		client.moveByAngleThrottle(0.0f, 0.0f, 0.7f, 0.0f, 0.2f);
		break;
	case 's':
		client.moveByAngleThrottle(0.0f, 0.0f, 0.5f, 0.0f, 0.2f);
		break;
	case 'a'://��תʱ���½�...
		client.moveByAngleThrottle(0.0f, 0.0f, 0.575, -1.0f, 0.2f);
		break;
	case 'd':
		client.moveByAngleThrottle(0.0f, 0.0f, 0.575, 1.0f, 0.2f);
		break;

	//�������Ի�ͷ����ǰ������
	case 'i'://pitch y����ʱ��Ƕ�
		client.moveByAngleThrottle(-0.1f, 0.0f, 0.575, 0.0f, 0.2f);
		break;
	case 'k'://pitch y����ʱ��Ƕ�
		client.moveByAngleThrottle(0.1f, 0.0f, 0.575, 0.0f, 0.2f);
		break;
	case 'j'://roll x����ʱ��Ƕ�
		client.moveByAngleThrottle(0.0f, -0.1f, 0.575, 0.0f, 0.2f);
		break;
	case 'l'://roll x����ʱ��Ƕ�
		client.moveByAngleThrottle(0.0f, 0.1f, 0.575, 0.0f, 0.2f);
		break;
	default:
		break;
	}

	key_value_cv = -1;
	return 0;
}







