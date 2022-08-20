
#include <mmdeviceapi.h>
#include <endpointvolume.h>

//for PKEY_Device_FriendlyName
#include <Functiondiscoverykeys_devpkey.h>

WCHAR *GetVolumeDeviceName(void *ptr)
{
    static WCHAR deviceName[255] = {0};
    HRESULT hr = S_OK;
    IPropertyStore *pProps = NULL;
    LPWSTR pwszID = NULL;
    IMMDevice *pDevice = (IMMDevice *)ptr;
    if (pDevice == NULL) return deviceName;

    //pDevice->GetId(&pwszID);
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr)) {
        PROPVARIANT varName;
        PropVariantInit(&varName);
        // Get the endpoint's friendly-name property.
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        if (SUCCEEDED(hr)) {
            // save endpoint friendly name and endpoint ID.
            lstrcpy(deviceName, varName.pwszVal);
        }
        PropVariantClear(&varName);
        //CoTaskMemFree(pwszID);
        pProps->Release();
    }
    return deviceName;
}

IAudioEndpointVolume *GetEndpointVolume()
{
    static IMMDeviceEnumerator *pEnumerator = NULL;
    HRESULT hr = S_OK;

    //    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    //    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
    if (!pEnumerator) {
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr)) {
            hr = CoCreateInstance(
                __uuidof(MMDeviceEnumerator),
                NULL, CLSCTX_ALL,
                __uuidof(IMMDeviceEnumerator),
                (void**)&pEnumerator);
        }
    }

    if (!pEnumerator) return NULL;

    IMMDevice *pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(
        eRender, eConsole, &pDevice);

    //pEnumerator->Release(); Keep static pEnumerator

    if (!pDevice) return NULL;

    IAudioEndpointVolume *pEndpointVolume = NULL;

    hr = pDevice->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_INPROC_SERVER, NULL,
        reinterpret_cast<void **>(&pEndpointVolume));
    GetVolumeDeviceName(pDevice);
    pDevice->Release();
    return pEndpointVolume;
}

int GetVolumeMute()
{
    BOOL currentMute = FALSE;
    IAudioEndpointVolume *pEndpointVolume = GetEndpointVolume();
    if (pEndpointVolume) {
        pEndpointVolume->GetMute(&currentMute);
    }
    return currentMute ? 1 : 0;
}

int SetVolumeMute(int mute)
{
    IAudioEndpointVolume *pEndpointVolume = GetEndpointVolume();
    if (pEndpointVolume) {
        BOOL bMute = (mute == 0) ? FALSE : TRUE;
        pEndpointVolume->SetMute(bMute, NULL);
        return 0;
    }
    return 1;
}

int GetVolumeLevel()
{
    HRESULT hr = S_OK;
    float fLevel = 0.0;
    IAudioEndpointVolume *pEndpointVolume = GetEndpointVolume();
    if (!pEndpointVolume) return 0;
    pEndpointVolume->GetMasterVolumeLevelScalar(&fLevel);
    if (pEndpointVolume) pEndpointVolume->Release();
    int iLevel = (int)(fLevel * 100);
    if (iLevel <= 0) return 0;
    if (iLevel >= 100) return 100;
    return iLevel;
}

int SetVolumeLevel(int iLevel)
{
    HRESULT hr = S_OK;
    if (iLevel < 0) iLevel = 0;
    else if (iLevel > 100) iLevel = 100;
    float fLevel = iLevel / 100.0f;
    IAudioEndpointVolume *pEndpointVolume = GetEndpointVolume();
    if (!pEndpointVolume) return 1;

    UINT iChannelSum = 0;
    if (!pEndpointVolume) return 1;
    hr = pEndpointVolume->GetChannelCount(&iChannelSum);

    if (SUCCEEDED(hr) && (iChannelSum >= 2)) {
        for (UINT i = 0; i < iChannelSum; i++) {
            pEndpointVolume->SetChannelVolumeLevelScalar(i, fLevel, NULL);
        }
    }
    if (pEndpointVolume) pEndpointVolume->Release();
    return 0;
}
