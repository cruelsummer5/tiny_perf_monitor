#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <deque>
#include <type_traits>
#include "PerfMonitor.h"
static constexpr int X_STEP = 60;

static constexpr int LOW_GRADE = 1000;
static constexpr int MEDIUM_GRADE = 2000;
static constexpr int HIGH_GRADE = 3000;

static const char* chartInfos[6][7] = {{ "%使用率", "60S", "100%", "0"},
							{ "%使用率", "60S", "100%", "0"},
							{ "吞吐量", "60S", "100kbps", "0"},
							{ "%s使用率", "60S", "100%", "0"},
							{ "%使用率", "60S", "100%", "0"},
							{ "吞吐量", "60S", "100kbps", "0"}};

static wxString initTitle = "";

static wxColour chartColors[6] = {wxColour(176, 215, 147) , wxColour(17, 125, 187),
									wxColour(200, 145, 96), wxColour(176, 215, 147),
									wxColour(149, 40, 180),wxColour(200, 145, 96)};


// ----------------------------------------------------------------------------
// curve chart that display 60 point with a value between 0 and 100 
// and the early inserted points will be on the left side to the late ones
// ----------------------------------------------------------------------------
class CurveChart : public wxPanel {
public:
	CurveChart(wxWindow* parent, const wxColour& colour) : wxPanel(parent),  _colour(colour){}

	void appendPoint(int i)
	{
		_points.push_back(i);
		if (_points.size() > X_STEP) _points.pop_front();
		Refresh();
	}

private:
	
	void OnPaint(wxPaintEvent& event) {
		wxPaintDC dc(this);

		wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
		gc->SetPen(wxPen(_colour, 2));
		gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

		int width, height;
		GetSize(&width, &height);
		gc->DrawRectangle(0, 0, width, height);

		if (_points.size() <= 0) return;
		double xStep = width * 1.0 / X_STEP;
		double yStep = height * 1.0 / 100;
		int lastX = 0, lastY = 0;

		gc->SetPen(wxPen(_colour, 1));
		for (size_t i = 0; i < _points.size(); ++i)
		{
			int x = width - i * xStep;
			int y = height - _points[_points.size() - 1 - i] * yStep;

			if (i > 0)
				gc->StrokeLine(lastX, lastY, x, y);
				
			lastX = x;
			lastY = y;

		}

		delete gc;
	}

	void OnSize(wxSizeEvent& event) {
		Refresh();
	}

	std::deque<int> _points;
	wxColour _colour;

	wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(CurveChart, wxPanel)
EVT_PAINT(CurveChart::OnPaint)
EVT_SIZE(CurveChart::OnSize)
wxEND_EVENT_TABLE()

// ----------------------------------------------------------------------------
// ChartWithTitle is a CurveChart with a title above 
// ----------------------------------------------------------------------------
class ChartWithTitle : public wxPanel {
public:
	ChartWithTitle(wxWindow* parent, const wxColour& colour, const wxString& chartLTStr, const wxString& chartLBStr,
		const wxString& chartRTStr, const wxString& chartRBStr) : wxPanel(parent) {

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
		chartTitleText = new wxStaticText(this, wxID_ANY, initTitle);
		topSizer->Add(chartTitleText, wxSizerFlags().Center().Border(wxTOP, 10));

		mainSizer->Add(topSizer, 0, wxEXPAND | wxALL, 2);

		// 创建wxGridSizer
		wxGridSizer* topGridSizer = new wxGridSizer(1, 2, 0, 0);

		// 创建wxStaticText，加入GridSizer
		wxStaticText* chartLTText = new wxStaticText(this, wxID_ANY, chartLTStr);
		topGridSizer->Add(chartLTText, 0, wxALIGN_LEFT | wxLEFT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, 5);

		wxStaticText* chartRTText = new wxStaticText(this, wxID_ANY, chartRTStr);
		topGridSizer->Add(chartRTText, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_VERTICAL, 5);

		// 加入BoxSizer
		mainSizer->Add(topGridSizer, 0, wxEXPAND);

		// 创建wxPanel，并加入BoxSizer
		chart = new CurveChart(this, colour);
		mainSizer->Add(chart, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

		// 创建wxGridSizer
		wxGridSizer* btmGridSizer = new wxGridSizer(1, 2, 0, 0);

		// 创建wxStaticText，加入GridSizer
		wxStaticText* chartLBText = new wxStaticText(this, wxID_ANY, chartLBStr);
		btmGridSizer->Add(chartLBText, 0, wxALIGN_LEFT | wxTOP | wxLEFT | wxALIGN_CENTER_VERTICAL, 5);

		wxStaticText* chartRBText = new wxStaticText(this, wxID_ANY, chartRBStr);
		btmGridSizer->Add(chartRBText, 0, wxALIGN_RIGHT | wxTOP | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
		mainSizer->Add(btmGridSizer, 0, wxEXPAND, 2);

		
		// 设置BoxSizer为主窗口的sizer
		SetSizer(mainSizer);
		mainSizer->Fit(this);
		Layout();
	}

	void appendPoint(int i)
	{
		wxString valStr = wxString::Format("%d", i);
		chart->appendPoint(i);
	}

	void changeTitle(const wxString& title)
	{
		chartTitleText->SetLabel(title);
		Layout();
	}
private:
	CurveChart* chart;
	wxStaticText* chartTitleText;
};


class MyFrame : public wxFrame {
public:
	MyFrame() : wxFrame(nullptr, wxID_ANY, "My Frame", wxDefaultPosition, wxSize(780, 500)), _monitor() {

		wxASSERT(_monitor.statusValid() == true);

		wxMenuBar *menuBar = new wxMenuBar();

		wxMenu *freqMenu = new wxMenu();
		freqMenu->AppendRadioItem(101, "1s");
		freqMenu->AppendRadioItem(102, "2s");
		freqMenu->AppendRadioItem(103, "3s");
		menuBar->Append(freqMenu, "Frequency");

		SetMenuBar(menuBar);

		Bind(wxEVT_MENU, &MyFrame::OnFreqSelected, this, 101, 103);
		// 创建两列三行的布局
		wxGridSizer* sizer = new wxGridSizer(2, 3, 0, 0);

		// 创建和添加监控面板
		for (int i = 0; i < std::extent<decltype(chartColors)>().value; i++) { 
			ChartWithTitle* panel = new ChartWithTitle(this, chartColors[i], chartInfos[i][0], chartInfos[i][1], chartInfos[i][2], chartInfos[i][3]); // 自定义面板类
			panel->SetMinSize(wxSize(250, 240));
			sizer->Add(panel, 1, wxEXPAND | wxALL, 5);
			panels.push_back(panel);
		}

		// 设置窗口布局
		SetSizer(sizer);
		Layout();

		_timer = new wxTimer(this);
		Connect(wxEVT_TIMER, wxTimerEventHandler(MyFrame::OnTimer), NULL, this);
		_timer->Start(LOW_GRADE);

		_monitor.start();

		SetTitle("tiny perf monitor");
	}

	~MyFrame()
	{
		delete _timer;
	}

private:

	void OnTimer(wxTimerEvent& event)
	{
		static int count = 0;

		_monitor.tick();
		RealTimeData rtd = _monitor.getDta();

		std::vector<int> dtaVec = {rtd.curProcessCpuUsage, rtd.iCPUUsage, rtd.ulbps / 1000, 
								rtd.curProcessMemUsage,rtd.MemUsage, rtd.dlbps / 1000};
		std::vector<wxString> titleVec = { wxString::Format("当前进程CPU使用率 %d%%", rtd.curProcessCpuUsage), 
			wxString::Format("系统CPU使用率 %d%%", rtd.iCPUUsage), 
			wxString::Format("网络上传速率 %dkbps", rtd.ulbps / 1000), 
			wxString::Format("当前进程内存使用率 %d%%(%d MB)", rtd.curProcessMemUsage, rtd.curProcessMem / 1000000),
			wxString::Format("系统内存使用率 %d%%", rtd.MemUsage),
			wxString::Format("网络下载速率 %dkbps", rtd.dlbps / 1000), };
		
		for (int i = 0; i < panels.size(); i++)
		{
			panels[i]->appendPoint(dtaVec[i]);
			panels[i]->changeTitle(titleVec[i]);
		}
		

		++count;
	}

	void OnFreqSelected(wxCommandEvent& event)
	{
		wxString freqStr;
		switch (event.GetId())
		{
		case 101:
			_timer->Start(LOW_GRADE);
			break;
		case 102:
			_timer->Start(MEDIUM_GRADE);
			break;
		case 103:
			_timer->Start(HIGH_GRADE);
			break;
		}
	}
	wxTimer* _timer;
	std::vector<ChartWithTitle*> panels;
	PerfMonitor _monitor;
};

class MyApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		
		MyFrame* frame = new MyFrame();
		frame->Show();
		return true;
	}
};


wxIMPLEMENT_APP(MyApp);
