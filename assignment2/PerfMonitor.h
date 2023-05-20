//
// PefMonitor.h
//
// $Id$
//

//
// <this is header file for PerfMonitor which can only be used on Windows. Can
// be omitted in source (.cpp) files.>
//usage below:
//PerfMonitor monitor;
//if (monitor.statusValid()) //to check if everything is Fine
//{
//monitor.start();
//sleep(1000);//must sleep over or equal to 1s
//monitor.tick();
//RealTimeData rtd = monitor.getDta();//RealTimeData contains data such system CPU/Memory Usage or Network Speed 
//}
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// All rights reserved.
//
// <License Notice>
//

#pragma once

#include <pdh.h>

struct RealTimeData
{
	int	iCPUUsage;			// CPUʹ����
	int	dlbps;		// ��������
	int	ulbps;			// �ϴ�����
	int MemUsage; //�ڴ�ʹ����
	int curProcessCpuUsage; //��ǰ���̵�cpuʹ����
	int curProcessMem; //��ǰ����ռ���ڴ��ֽ���
	int curProcessMemUsage; //��ǰ�����ڴ��ʹ����
};

class PerfMonitor
{
public:
	PerfMonitor();
	void start();
	void tick();
	bool statusValid();
	RealTimeData getDta();
	~PerfMonitor();
private:
	PDH_STATUS status;
	HQUERY query;
	HCOUNTER totalCpu, totalMem;
	HCOUNTER sent, rcv;
	HCOUNTER curProcessCpu, curProcessMem;
	RealTimeData rtd;
};
