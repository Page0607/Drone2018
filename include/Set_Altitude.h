#pragma once
#ifndef _SET_ALLTITUDE_H
#define _SET_ALLTITUDE_H
#include <iostream>
#include "Drone_data.h"
#include "PID.h"

/*
����ֵ��0---����Ŀ��߶ȣ������˳�    1---û�ж���Ŀ��߶ȣ��쳣�˳�
target_attlitude---Ŀ��߶�
mode---���߷�ʽ��1��ʾ����   2��ʾ�ֵ�
*/
extern msr::airlib::MultirotorRpcLibClient client;

int setAltitude(float target_altitude, int mode)
{
	int result_set_attlitude = 1;
	float allowable_error;	//��������
	float current_error;	//cirrent_error=target_attlitude-current_attlitude
	PID PID_temp;

	if (mode == 1)
	{
		allowable_error = 1.0;
	}
	else
	{
		allowable_error = 2.0;
	}

	//���ִ��20�Σ������쳣�˳�
	int i = 0;
	while(true)
	{
		std::cout << "��" << (i++) << "��adjust altitude" << endl;
		current_error = target_altitude - Barometer_data.altitude;

		if (abs(current_error) < allowable_error)
		{
			result_set_attlitude = 0;
			break;
		}

		//���Ƹ߶�
		double throttle_increment = PID_temp.PIDZ(target_altitude, allowable_error);
		if (throttle_increment > allowable_error*0.1 || throttle_increment < (-allowable_error*0.1))
		{
			client.moveByAngleThrottle(0.0f, 0.0f, 0.5875 + throttle_increment, 0.0f, 0.7f);
		}
	}

	return result_set_attlitude;
}

#endif _SET_ALLTITUDE_H
