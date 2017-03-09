#pragma once

#include <vector>
#include <WinDef.h>
#include <tchar.h>
using std::vector;

using namespace std;

#define MAX_MONITOR_NAME 256

static std::vector<HMONITOR> g_hMonitorGroup;

// 显示器模式信息
typedef struct MonitorModeInfo_t
{
    unsigned int m_nWidth;
    unsigned int m_nHeight;

    MonitorModeInfo_t(int nWidth, int nHeight) : m_nWidth(nWidth), m_nHeight(nHeight) {}
}MonitorModeInfo;

// 显示器信息
struct MonitorInfo
{
    TCHAR szDevice[MAX_MONITOR_NAME];                   // 显示器名称
    std::vector<MonitorModeInfo> m_vecModeInfo;         // 当前名称的显示器支持的分辨率模式
};

typedef std::vector<MonitorInfo> VEC_MONITORMODE_INFO;  // 所有的显示器信息


class MonitorAdapter
{
public:
    MonitorAdapter();
    ~MonitorAdapter();

    // 回调函数
    static int CALLBACK MonitorEnumProc(HMONITOR hMonitor,
        HDC hdc,
        LPRECT lpRMonitor,
        LPARAM dwData);

    // 得到所有显示器的名称
    void GetAllMonitorName(VEC_MONITORMODE_INFO& m_vecMonitorListInfo);

    void GetDisplayModeByMonitorList(VEC_MONITORMODE_INFO& vecMonitorListInfo);

    // 得到所有显示器的模式
    void GetAllDisplayMode(VEC_MONITORMODE_INFO& m_vecMonitorListInfo);

    // 得到当前显示器的模式
    void GetCurrentDisplayMode(VEC_MONITORMODE_INFO& m_vecMonitorListInfo);

    //得到屏幕当前分辨率
    void GetCurrentReselotion(int& nWidth, int& nHeight, int& nFreq, int& nBits);

    //根据屏幕ID取获取屏幕的对应分辨率
    void GetCurrentReselotion(LPCWSTR lpszDeviceName, int& nWidth, int& nHeight, int& nFreq, int& nBits);

    //修改分辨率
    int ChangeMonitorReselotion(HMONITOR hMonitor, const int nWidth, const int nHeight, const int nFre = -1, const int nColorBits = -1);

};
