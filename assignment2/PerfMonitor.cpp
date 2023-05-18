#include "PerfMonitor.h"
#include <string>
#include <IPHlpApi.h>
#include <algorithm>
#include <cassert>
#pragma comment(lib,"pdh.lib")
#pragma comment(lib, "IPHlpApi.lib")
using namespace std;

static MEMORYSTATUSEX memoryStatus;



string getProcessName()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	string path(buffer);
	size_t pos = path.find_last_of("\\");
	if (pos != wstring::npos) {
		path = path.substr(pos + 1);
	}

	size_t dotpos = path.find_last_of(".");
	if (dotpos != string::npos) {
		path = path.substr(0, dotpos);
	}
	return path;
}

string getCurNetInterface() {
	ULONG ulSize = 0;
	IP_ADAPTER_INFO *pAdapter = nullptr;
	if (GetAdaptersInfo(pAdapter, &ulSize) == ERROR_BUFFER_OVERFLOW) {
		pAdapter = (IP_ADAPTER_INFO*)new char[ulSize];
	}
	else {
		//cout << "GetAdaptersInfo fail" << endl;
		return "";
	}

	if (GetAdaptersInfo(pAdapter, &ulSize) != ERROR_SUCCESS) {
		//cout << "GetAdaptersInfo fail" << endl;
		return "";
	}

	IPAddr ipAddr = { 0 };
	DWORD dwIndex = -1;
	DWORD nRet = GetBestInterface(ipAddr, &dwIndex);
	assert(NO_ERROR == nRet);

	string strInterface;
	for (auto *pCur = pAdapter; pCur != NULL; pCur = pCur->Next) {
		//if (pCur->Type != MIB_IF_TYPE_ETHERNET)
		//  continue;

		if (pCur->Index == dwIndex) {
			//cout << "Best Interface!! ";
			strInterface = pCur->Description;
		}

		//cout << "Descrip: " << pCur->Description;
		//cout << ", Name: " << pCur->AdapterName << endl;
		//cout << "IP: " << pCur->IpAddressList.IpAddress.String;
		//cout << ", Gateway: " << pCur->GatewayList.IpAddress.String << endl << endl;
	}

	delete pAdapter;
	return strInterface;
}

PerfMonitor::PerfMonitor() : query(NULL),
	totalCpu(NULL),
	totalMem(NULL),
	sent(NULL),
	rcv(NULL),
	curProcessCpu(NULL),
	curProcessMem(NULL)
{
	// 打开总CPU使用率计数器
	query = NULL;
	status = PdhOpenQuery(NULL, NULL, &query);
	if (status != ERROR_SUCCESS) return;
	//添加全局的cpu计数器
	status = PdhAddCounter(query, "\\Processor(_Total)\\% Processor Time", 0, &totalCpu);
	if (status != ERROR_SUCCESS) return;

	memoryStatus.dwLength = sizeof(memoryStatus);
	// 获取系统内存状态
	bool res = GlobalMemoryStatusEx(&memoryStatus);
	assert(res);

	//添加全局内存计数器
	status = PdhAddCounter(query, "\\Memory\\Available Bytes", NULL, &totalMem);
	if (status != ERROR_SUCCESS) return;

	string netIntStr = getCurNetInterface();
	replace(netIntStr.begin(), netIntStr.end(), '(', '[');
	replace(netIntStr.begin(), netIntStr.end(), ')', ']');
	string strInterface = "\\Network Interface(" + netIntStr + ")\\";


	// 添加上传速度计数器
	status = PdhAddCounter(query, string(strInterface + "Bytes Sent/sec").c_str(), 0, &sent);
	if (status != ERROR_SUCCESS) return;

	// 添加下载速度计数器
	status = PdhAddCounter(query, string(strInterface + "Bytes Received/sec").c_str(), 0, &rcv);
	if (status != ERROR_SUCCESS) return;

	string pname = getProcessName();

	//添加当前进程的工作集计数器
	string pMemPath = string("\\Process(" + pname + ")\\Working Set");
	status = PdhAddCounter(query, pMemPath.c_str(), NULL, &curProcessMem);
	if (status != ERROR_SUCCESS) return;

	//添加当前进程的cpu使用率计数器
	string pCpuPath = "\\Process(" + pname + ")\\% Processor Time";
	status = PdhAddCounter(query, pCpuPath.c_str(), NULL, &curProcessCpu);
	if (status != ERROR_SUCCESS) return;
}

void PerfMonitor::start()
{
	status = PdhCollectQueryData(query);
}

void PerfMonitor::tick()
{
	status = PdhCollectQueryData(query);

	memset(&rtd, 0, sizeof(RealTimeData));
	
	PDH_FMT_COUNTERVALUE counterValue;

	//获取总cpu使用率
	PdhGetFormattedCounterValue(totalCpu, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.iCPUUsage = counterValue.doubleValue;

	//获取总内存使用率
	PdhGetFormattedCounterValue(totalMem, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.MemUsage = (1 - counterValue.doubleValue / memoryStatus.ullTotalPhys) * 100;

	//获取上传速率
	PdhGetFormattedCounterValue(sent, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.ulbps = counterValue.doubleValue / 8;

	//获取下载速率
	PdhGetFormattedCounterValue(rcv, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.dlbps = counterValue.doubleValue / 8;

	//获取当前进程的内存使用率和使用字节数
	PdhGetFormattedCounterValue(curProcessMem, PDH_FMT_LONG, NULL, &counterValue);
	rtd.curProcessMem = counterValue.longValue;
	rtd.curProcessMemUsage = counterValue.longValue * 1.0 / memoryStatus.ullTotalPhys;

	//获取当前进程的cpu使用率
	PdhGetFormattedCounterValue(curProcessCpu, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.curProcessCpuUsage = counterValue.doubleValue;
}

bool PerfMonitor::statusValid()
{
	return status == ERROR_SUCCESS;
		;
}

RealTimeData PerfMonitor::getDta()
{
	return rtd;
}

PerfMonitor::~PerfMonitor()
{
	if (query != NULL) PdhCloseQuery(query);
}
