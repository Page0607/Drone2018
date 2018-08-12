//airsim
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "api/RpcLibClientBase.hpp"
//opencv
#include "opencv2/opencv.hpp"

#include <iostream>
#include "WINDOWS.h"

#include "timer.h" 
#include <pthread.h>

//�����ռ�
//using namespace std;
using namespace cv;
using namespace msr::airlib;

//airsim ���
msr::airlib::MultirotorRpcLibClient client("localhost", 41451, 60000);//����localhost:41451 	
typedef ImageCaptureBase::ImageRequest ImageRequest;
typedef ImageCaptureBase::ImageResponse ImageResponse;
typedef ImageCaptureBase::ImageType ImageType;

//�߳�
DWORD WINAPI Key_Scan(LPVOID pVoid);//__stdcall �����ڷ��ص�������֮ǰ��������ջ��ɾ��
DWORD WINAPI get_img(LPVOID pVoid);
DWORD WINAPI Control_Z_Thread(LPVOID pVoid);//��������߶�
HANDLE hTimer1;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;//������					

//����
volatile int key_value_cv = -1;
static int key_control(int key);//��������

//ͼ��
cv::Mat front_image, down_image,test_image;

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
				//�ݻ�����OpenCV����
				cv::destroyAllWindows();
				//�ر��߳�
				CloseHandle(hThread1);
				CloseHandle(hThread2);
				CloseHandle(hThread3);
				//�رն�ʱ��
				CloseHandle(hTimer1);

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
	}

}

DWORD WINAPI Key_Scan(LPVOID pVoid)
{
	clock_t time_1;// = clock();//get time
	
	while (true)
	{
		//�ȴ���ʱ��ʱ�䵽��
		WaitForSingleObject(hTimer1, INFINITE);

		pthread_mutex_lock(&mutex1);//�����

		//��ʾͼ��
		if (!front_image.empty())
		{
			cv::imshow("FROWARD", front_image);
		}
		if (!down_image.empty())
		{
			cv::imshow("DOWN", down_image);
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

		pthread_mutex_unlock(&mutex1);//�ͷ���
	}
	return 0;
}

DWORD WINAPI get_img(LPVOID pVoid)
{
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
			time_1 = clock();//get time

			std::vector<ImageRequest> request;
			request = { ImageRequest(0, ImageType::Scene) , ImageRequest(0, ImageType::DepthPerspective, true), ImageRequest(3, ImageType::Scene) };

			std::vector<ImageResponse>& response = client.simGetImages(request);

			if (response.size() > 0)
			{
				front_image = cv::imdecode(response.at(0).image_data_uint8, cv::IMREAD_COLOR);	//;ǰ��ͼ
				down_image = cv::imdecode(response.at(2).image_data_uint8, cv::IMREAD_COLOR);	//����ͼ	
			}
		}
		pthread_mutex_unlock(&mutex1);//�ͷ���
	}
	return 0;
}

static int key_control(int key)//��������
{
	clock_t time_1;// = clock();//get time

	pthread_mutex_lock(&mutex1);//�����
	switch (key)
	{
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
	pthread_mutex_unlock(&mutex1);//�ͷ���
	return 0;
}







