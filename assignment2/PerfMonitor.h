#pragma once

#include <pdh.h>

struct RealTimeData
{
	int	iCPUUsage;			// CPU使用率
	int	dlbps;		// 下载速率
	int	ulbps;			// 上传速率
	int MemUsage; //内存使用率
	int curProcessCpuUsage; //当前进程的cpu使用率
	int curProcessMem; //当前进程占有内存字节数
	int curProcessMemUsage; //当前进程内存的使用率
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
