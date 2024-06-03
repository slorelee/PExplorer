
#include <Windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "propsys.lib")
#include <tchar.h>
#include "DiskHelper.h"
#include "../DUI/Helper.h"

INT GetVolumeBitLockerProtection(LPCWSTR parsingName)
{
    INT rc = 0;
    TCHAR szFs[32] = _T("");

    if (!GetVolumeInformation(parsingName, NULL, 0,
        NULL, NULL, NULL, szFs, sizeof(szFs))) {
        rc = GetLastError();
        if (rc == (INT)0x80310000) rc = 6;
    }

    char buff[1024] = { 0 };
    sprintf(buff, "GetVolumeInformation() = %d", rc);
    LOGA(buff);

    /*
    TCHAR szNtDeviceName[MAX_PATH];
    TCHAR szDriveLetter[3];

    szDriveLetter[0] = parsingName[0];
    szDriveLetter[1] = TEXT(':');
    szDriveLetter[2] = TEXT('\0');

    if (QueryDosDevice(szDriveLetter, szNtDeviceName, MAX_PATH)) {
        HANDLE       hDevice;
        TCHAR        szDriveName[7]; // holds \\.\X: plus NULL.
        TCHAR szFs[32] = _T("");
        _tprintf(TEXT("%s is mapped to %s\n"), szDriveLetter, szNtDeviceName);

        _tcsncpy_s(szDriveName, _countof(szDriveName), _T("\\\\.\\"), _TRUNCATE);
        _tcsncat_s(szDriveName, _countof(szDriveName), szDriveLetter, _TRUNCATE);
    }
    */
    return rc;
}
INT GetBitLockerProtectionStatus(LPCWSTR parsingName)
{
    IShellItem2 *drive = NULL;
    HRESULT hr = 0;
    INT rc = GetVolumeBitLockerProtection(parsingName);
    if (rc != 0) return rc;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    hr = SHCreateItemFromParsingName(parsingName, NULL, IID_PPV_ARGS(&drive));
    if (SUCCEEDED(hr)) {
        PROPERTYKEY pKey;
        LOGA("PSGetPropertyKeyFromName().");
        hr = PSGetPropertyKeyFromName(L"System.Volume.BitLockerProtection", &pKey);
        if (SUCCEEDED(hr)) {
            PROPVARIANT prop;
            PropVariantInit(&prop);
            LOGA("drive->GetProperty().");
            hr = drive->GetProperty(pKey, &prop);
            if (SUCCEEDED(hr)) {
                INT status = prop.intVal;
                LOGA("prop.intVal.");
                drive->Release();
                return status;
            }
        }
    }
    if (drive) drive->Release();
    return 0;
}

DriveEncryptionStatus GetDriveEncryptionStatus(LPCWSTR parsingName)
{
    INT status = GetBitLockerProtectionStatus(parsingName);
    if (status == 0) return DriveEncryptionStatus::UnknownStatus;
    if (status == 6) return DriveEncryptionStatus::Locked;
    if (status == 1 || status == 3 || status == 5) {
        return DriveEncryptionStatus::Protected;
    } else {
       return DriveEncryptionStatus::Unprotected;
    }
}


