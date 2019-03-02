#pragma once

#include <endpointvolume.h>

extern IAudioEndpointVolume *GetEndpointVolume();
extern WCHAR *GetVolumeDeviceName(void *ptr);
extern int GetVolumeMute();
extern int SetVolumeMute(int mute);
extern int GetVolumeLevel();
extern int SetVolumeLevel(int iLevel);
