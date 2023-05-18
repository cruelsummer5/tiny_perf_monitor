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
