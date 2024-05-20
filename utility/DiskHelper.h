#pragma once

enum DriveEncryptionStatus {
    Unprotected,
    Protected,
    Locked,
    UnknownStatus
};

extern INT GetBitLockerProtectionStatus(LPCWSTR parsingName);
extern DriveEncryptionStatus GetDriveEncryptionStatus(LPCWSTR parsingName);
