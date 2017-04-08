#include <precomp.h>
#include "MonitorAdapter.h"

MonitorAdapter::MonitorAdapter(void)
{
}

MonitorAdapter::~MonitorAdapter(void)
{
}

int CALLBACK MonitorAdapter::MonitorEnumProc(HMONITOR hMonitor,
    HDC hdc,
    LPRECT lpRMonitor,
    LPARAM dwData)
{
    g_hMonitorGroup.push_back(hMonitor);
    return 1;
}


// 得到所有显示器的名称
void MonitorAdapter::GetAllMonitorName(VEC_MONITORMODE_INFO& vecMonitorListInfo)
{
    g_hMonitorGroup.clear();
    ::EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    //vector<HMONITOR>::iterator ithMoniter = g_hMonitorGroup.begin();
    for (int i = 0; i < g_hMonitorGroup.size(); i++)
    {
        MONITORINFOEX mixTemp;
        memset(&mixTemp, 0, sizeof(MONITORINFOEX));
        mixTemp.cbSize = sizeof(MONITORINFOEX);

        GetMonitorInfo(g_hMonitorGroup[i], &mixTemp);
        VEC_MONITORMODE_INFO::iterator itBeg = vecMonitorListInfo.begin();
        VEC_MONITORMODE_INFO::iterator itEnd = vecMonitorListInfo.end();
        for (; itBeg != itEnd; ++itBeg)
        {
            if (0 == _tcscmp(mixTemp.szDevice, itBeg->szDevice))
            {
                break;
            }
        }

        //没有在列表中找到,则需要添加
        if (itBeg == itEnd)
        {
            MonitorInfo tmpMonitorInfo;
            _tcscpy_s(tmpMonitorInfo.szDevice, sizeof(tmpMonitorInfo.szDevice), mixTemp.szDevice);
            vecMonitorListInfo.push_back(tmpMonitorInfo);
        }
    }
}


// 得到所有显示器的模式
void MonitorAdapter::GetDisplayModeByMonitorList(VEC_MONITORMODE_INFO& vecMonitorListInfo)
{
    BOOL bRetVal;
    DEVMODE devmode;

    VEC_MONITORMODE_INFO::iterator itBeg = vecMonitorListInfo.begin();
    VEC_MONITORMODE_INFO::iterator itEnd = vecMonitorListInfo.end();
    for (NULL; itBeg != itEnd; ++itBeg)
    {
        int iMode = 0;
        do
        {
            if (itBeg->szDevice[0] == _T('\0')) {
                bRetVal = ::EnumDisplaySettings(NULL, iMode, &devmode);
            } else {
                bRetVal = ::EnumDisplaySettings(itBeg->szDevice, iMode, &devmode);
            }
            iMode++;
            if (bRetVal)
            {
                BOOL bFind = FALSE;

                vector<MonitorModeInfo>::iterator itBeg_Mode = itBeg->m_vecModeInfo.begin();
                vector<MonitorModeInfo>::iterator itEnd_Mode = itBeg->m_vecModeInfo.end();
                for (NULL; itBeg_Mode != itEnd_Mode; ++itBeg_Mode)
                {
                    // 如果已经在列表中找到,则结束本次循环
                    if ((itBeg_Mode->m_nWidth == devmode.dmPelsWidth) && (itBeg_Mode->m_nHeight == devmode.dmPelsHeight))
                    {
                        bFind = TRUE;
                        break;
                    }

                    // 插入数据时, 从 大到小排列 (按windows 分辨率设置,优先比较 宽)
                    if (
                        (itBeg_Mode->m_nWidth < devmode.dmPelsWidth) ||
                        ((itBeg_Mode->m_nWidth == devmode.dmPelsWidth) && (itBeg_Mode->m_nHeight < devmode.dmPelsHeight))
                        )
                    {
                        break;
                    }
                }

                if (!bFind)
                {
                    if (itBeg_Mode == itEnd_Mode)
                    {
                        itBeg->m_vecModeInfo.push_back(MonitorModeInfo(devmode.dmPelsWidth, devmode.dmPelsHeight));
                    }
                    else
                    {
                        itBeg->m_vecModeInfo.insert(itBeg_Mode, MonitorModeInfo(devmode.dmPelsWidth, devmode.dmPelsHeight));
                    }
                }
            }
        } while (bRetVal);
    }
}

// 得到所有显示器的模式
void MonitorAdapter::GetAllDisplayMode(VEC_MONITORMODE_INFO& vecMonitorListInfo)
{
    GetAllMonitorName(vecMonitorListInfo);
    GetDisplayModeByMonitorList(vecMonitorListInfo);
}

void MonitorAdapter::GetCurrentDisplayMode(VEC_MONITORMODE_INFO& vecMonitorListInfo)
{
    MonitorInfo tmpMonitorInfo = {0};
    tmpMonitorInfo.szDevice[0] = _T('\0');
    vecMonitorListInfo.push_back(tmpMonitorInfo);
    GetDisplayModeByMonitorList(vecMonitorListInfo);
}



void GetCurrentMonitorInfo(HMONITOR hMonitor, MONITORINFOEX *pmi)
{
    pmi->cbSize = sizeof(pmi);
    GetMonitorInfo(hMonitor, pmi);
}

HRESULT SetNewResolution(DEVMODE &dmOld, DEVMODE &dmNew)
{
    if (!(ChangeDisplaySettings(&dmNew, CDS_UPDATEREGISTRY)))
    {
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmOld);
        dmNew = dmOld;
        return S_OK;
    } else {
        ChangeDisplaySettings(&dmOld, CDS_UPDATEREGISTRY);
        dmNew = dmOld;
        return S_FALSE;
    }
}

int MonitorAdapter::ChangeMonitorResolution(HMONITOR hMonitor, const int nWidth, const int nHeight, const int nFre, const int nColorBits)
{
    TCHAR *pszDevice = NULL;
    MONITORINFOEX mi;

    if (NULL != hMonitor)
    {
        GetCurrentMonitorInfo(hMonitor, &mi);
        pszDevice = mi.szDevice;
    }

    DEVMODE dmOld = { 0 };
    dmOld.dmSize = sizeof(DEVMODE);

    BOOL bFlag = TRUE;
    bFlag = EnumDisplaySettings(pszDevice, ENUM_CURRENT_SETTINGS, &dmOld);
    if (bFlag != TRUE)
    {
        return -1;
    }

    if (dmOld.dmPelsWidth == nWidth && dmOld.dmPelsHeight == nHeight)
    {
        return 0;
    }

    DEVMODE dmNew = dmOld;

    dmNew.dmPelsWidth = nWidth;
    dmNew.dmPelsHeight = nHeight;

    HRESULT lResult = S_OK;
    lResult = SetNewResolution(dmOld, dmNew);
    if (!lResult) return -1;
    return 0;
}

void MonitorAdapter::GetCurrentResolution(int& nWidth, int& nHeight, int& nFreq, int& nBits)
{
    DEVMODE DeviceMode;
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DeviceMode);
    nWidth = DeviceMode.dmPelsWidth;
    nHeight = DeviceMode.dmPelsHeight;
    nFreq = DeviceMode.dmDisplayFrequency;
    nBits = DeviceMode.dmBitsPerPel;
}

void MonitorAdapter::GetCurrentResolution(LPCWSTR lpszDeviceName, int& nWidth, int& nHeight, int& nFreq, int& nBits)
{
    DEVMODE DeviceMode;
    EnumDisplaySettings(lpszDeviceName, ENUM_CURRENT_SETTINGS, &DeviceMode);
    nWidth = DeviceMode.dmPelsWidth;
    nHeight = DeviceMode.dmPelsHeight;
    nFreq = DeviceMode.dmDisplayFrequency;
    nBits = DeviceMode.dmBitsPerPel;
}

DWORD GetScreenRotation()
{
    DEVMODE tempdevmode;
    EnumDisplaySettings(NULL, -1, &tempdevmode);
    return tempdevmode.dmDisplayOrientation;
}

void SetScreenRotation(DWORD orientation)
{

    DEVMODE tempdevmode;
    EnumDisplaySettings(NULL, -1, &tempdevmode);

    //Do not rotate if the given value is the current screen orientation
    if (tempdevmode.dmDisplayOrientation == orientation) return;
    DWORD def_height;
    DWORD def_width;

    //Get base resolution settings
    if ((tempdevmode.dmDisplayOrientation == DMDO_DEFAULT) || (tempdevmode.dmDisplayOrientation == DMDO_180))
    {
        def_height = tempdevmode.dmPelsHeight;
        def_width = tempdevmode.dmPelsWidth;
    } else {
        def_height = tempdevmode.dmPelsWidth;
        def_width = tempdevmode.dmPelsHeight;
    }

    //Rotate if necessary
    if ((orientation == DMDO_90) || (orientation == DMDO_270))
    {
        tempdevmode.dmPelsWidth = def_height;
        tempdevmode.dmPelsHeight = def_width;
    } else {
        tempdevmode.dmPelsWidth = def_width;
        tempdevmode.dmPelsHeight = def_height;
    }

    tempdevmode.dmDisplayOrientation = orientation;
    ChangeDisplaySettings(&tempdevmode, 0);
}
