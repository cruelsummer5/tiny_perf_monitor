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
	// ����CPUʹ���ʼ�����
	query = NULL;
	status = PdhOpenQuery(NULL, NULL, &query);
	if (status != ERROR_SUCCESS) return;
	//���ȫ�ֵ�cpu������
	status = PdhAddCounter(query, "\\Processor(_Total)\\% Processor Time", 0, &totalCpu);
	if (status != ERROR_SUCCESS) return;

	memoryStatus.dwLength = sizeof(memoryStatus);
	// ��ȡϵͳ�ڴ�״̬
	bool res = GlobalMemoryStatusEx(&memoryStatus);
	assert(res);

	//���ȫ���ڴ������
	status = PdhAddCounter(query, "\\Memory\\Available Bytes", NULL, &totalMem);
	if (status != ERROR_SUCCESS) return;

	string netIntStr = getCurNetInterface();
	replace(netIntStr.begin(), netIntStr.end(), '(', '[');
	replace(netIntStr.begin(), netIntStr.end(), ')', ']');
	string strInterface = "\\Network Interface(" + netIntStr + ")\\";


	// ����ϴ��ٶȼ�����
	status = PdhAddCounter(query, string(strInterface + "Bytes Sent/sec").c_str(), 0, &sent);
	if (status != ERROR_SUCCESS) return;

	// ��������ٶȼ�����
	status = PdhAddCounter(query, string(strInterface + "Bytes Received/sec").c_str(), 0, &rcv);
	if (status != ERROR_SUCCESS) return;

	string pname = getProcessName();

	//��ӵ�ǰ���̵Ĺ�����������
	string pMemPath = string("\\Process(" + pname + ")\\Working Set");
	status = PdhAddCounter(query, pMemPath.c_str(), NULL, &curProcessMem);
	if (status != ERROR_SUCCESS) return;

	//��ӵ�ǰ���̵�cpuʹ���ʼ�����
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

	//��ȡ��cpuʹ����
	PdhGetFormattedCounterValue(totalCpu, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.iCPUUsage = counterValue.doubleValue;

	//��ȡ���ڴ�ʹ����
	PdhGetFormattedCounterValue(totalMem, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.MemUsage = (1 - counterValue.doubleValue / memoryStatus.ullTotalPhys) * 100;

	//��ȡ�ϴ�����
	PdhGetFormattedCounterValue(sent, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.ulbps = counterValue.doubleValue / 8;

	//��ȡ��������
	PdhGetFormattedCounterValue(rcv, PDH_FMT_DOUBLE, NULL, &counterValue);
	rtd.dlbps = counterValue.doubleValue / 8;

	//��ȡ��ǰ���̵��ڴ�ʹ���ʺ�ʹ���ֽ���
	PdhGetFormattedCounterValue(curProcessMem, PDH_FMT_LONG, NULL, &counterValue);
	rtd.curProcessMem = counterValue.longValue;
	rtd.curProcessMemUsage = counterValue.longValue * 1.0 / memoryStatus.ullTotalPhys;

	//��ȡ��ǰ���̵�cpuʹ����
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
