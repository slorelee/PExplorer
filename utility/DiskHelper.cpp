
#include <Windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "propsys.lib")

#include "DiskHelper.h"

INT GetBitLockerProtectionStatus(LPCWSTR parsingName)
{
    IShellItem2 *drive = NULL;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    hr = SHCreateItemFromParsingName(parsingName, NULL, IID_PPV_ARGS(&drive));
    if (SUCCEEDED(hr)) {
        PROPERTYKEY pKey;
        hr = PSGetPropertyKeyFromName(L"System.Volume.BitLockerProtection", &pKey);
        if (SUCCEEDED(hr)) {
            PROPVARIANT prop;
            PropVariantInit(&prop);
            hr = drive->GetProperty(pKey, &prop);
            if (SUCCEEDED(hr)) {
                INT status = prop.intVal;
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


