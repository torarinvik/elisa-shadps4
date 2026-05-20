// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <array>
#include "common/cstring.h"
#include "core/libraries/save_data/savedata.h"

namespace Libraries::SaveData {
constexpr size_t OrbisSaveDataMountPointDataMaxsize = 16;
using OrbisSaveDataBlocks = u64;
enum class OrbisSaveDataMountMode : u32 {
    RDONLY = 1 << 0,
    RDWR = 1 << 1,
    CREATE = 1 << 2,
    DESTRUCT_OFF = 1 << 3,
    COPY_ICON = 1 << 4,
    CREATE2 = 1 << 5,
};
enum class OrbisSaveDataMountStatus : u32 {
    NOTHING = 0,
    CREATED = 1,
};
enum class OrbisSaveDataParamType : u32 {
    ALL = 0,
    TITLE = 1,
    SUB_TITLE = 2,
    DETAIL = 3,
    USER_PARAM = 4,
    MTIME = 5,
};
struct OrbisSaveDataMount2 {
    Libraries::UserService::OrbisUserServiceUserId userId;
    s32 : 32;
    const OrbisSaveDataDirName* dirName;
    OrbisSaveDataBlocks blocks;
    OrbisSaveDataMountMode mountMode;
    std::array<u8, 32> _reserved;
    s32 : 32;
};
struct OrbisSaveDataMountPoint {
    Common::CString<OrbisSaveDataMountPointDataMaxsize> data;
};
struct OrbisSaveDataMountResult {
    OrbisSaveDataMountPoint mount_point;
    OrbisSaveDataBlocks required_blocks;
    u32 _unused;
    OrbisSaveDataMountStatus mount_status;
    std::array<u8, 28> _reserved;
    s32 : 32;
};
struct OrbisSaveDataMountInfo {
    OrbisSaveDataBlocks blocks;
    OrbisSaveDataBlocks freeBlocks;
    std::array<u8, 32> _reserved;
};
} // namespace Libraries::SaveData

extern "C" {

int SaveDataElisa_sceSaveDataAbort() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataAbort());
}

int SaveDataElisa_sceSaveDataBackup(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataBackup(static_cast<const Libraries::SaveData::OrbisSaveDataBackup*>(a0)));
}

int SaveDataElisa_sceSaveDataBindPsnAccount() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataBindPsnAccount());
}

int SaveDataElisa_sceSaveDataBindPsnAccountForSystemBackup() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataBindPsnAccountForSystemBackup());
}

int SaveDataElisa_sceSaveDataChangeDatabase() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataChangeDatabase());
}

int SaveDataElisa_sceSaveDataChangeInternal() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataChangeInternal());
}

int SaveDataElisa_sceSaveDataCheckBackupData(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckBackupData(static_cast<const Libraries::SaveData::OrbisSaveDataCheckBackupData*>(a0)));
}

int SaveDataElisa_sceSaveDataCheckBackupDataForCdlg() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckBackupDataForCdlg());
}

int SaveDataElisa_sceSaveDataCheckBackupDataInternal() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckBackupDataInternal());
}

int SaveDataElisa_sceSaveDataCheckCloudData() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckCloudData());
}

int SaveDataElisa_sceSaveDataCheckIpmiIfSize() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckIpmiIfSize());
}

int SaveDataElisa_sceSaveDataCheckSaveDataBroken() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckSaveDataBroken());
}

int SaveDataElisa_sceSaveDataCheckSaveDataVersion() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckSaveDataVersion());
}

int SaveDataElisa_sceSaveDataCheckSaveDataVersionLatest() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCheckSaveDataVersionLatest());
}

int SaveDataElisa_sceSaveDataClearProgress() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataClearProgress());
}

int SaveDataElisa_sceSaveDataCopy5() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCopy5());
}

int SaveDataElisa_sceSaveDataCreateUploadData() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataCreateUploadData());
}

int SaveDataElisa_sceSaveDataDebug() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebug());
}

int SaveDataElisa_sceSaveDataDebugCleanMount() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebugCleanMount());
}

int SaveDataElisa_sceSaveDataDebugCompiledSdkVersion() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebugCompiledSdkVersion());
}

int SaveDataElisa_sceSaveDataDebugCreateSaveDataRoot() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebugCreateSaveDataRoot());
}

int SaveDataElisa_sceSaveDataDebugGetThreadId() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebugGetThreadId());
}

int SaveDataElisa_sceSaveDataDebugRemoveSaveDataRoot() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebugRemoveSaveDataRoot());
}

int SaveDataElisa_sceSaveDataDebugTarget() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDebugTarget());
}

int SaveDataElisa_sceSaveDataDelete(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDelete(static_cast<const Libraries::SaveData::OrbisSaveDataDelete*>(a0)));
}

int SaveDataElisa_sceSaveDataDelete5() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDelete5());
}

int SaveDataElisa_sceSaveDataDeleteAllUser() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDeleteAllUser());
}

int SaveDataElisa_sceSaveDataDeleteCloudData() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDeleteCloudData());
}

int SaveDataElisa_sceSaveDataDeleteUser() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDeleteUser());
}

int SaveDataElisa_sceSaveDataDirNameSearch(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDirNameSearch(static_cast<const Libraries::SaveData::OrbisSaveDataDirNameSearchCond*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataDirNameSearchResult*>(a1)));
}

int SaveDataElisa_sceSaveDataDirNameSearchInternal() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDirNameSearchInternal());
}

int SaveDataElisa_sceSaveDataDownload() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataDownload());
}

int SaveDataElisa_sceSaveDataGetAllSize() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetAllSize());
}

int SaveDataElisa_sceSaveDataGetAppLaunchedUser() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetAppLaunchedUser());
}

int SaveDataElisa_sceSaveDataGetAutoUploadConditions() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetAutoUploadConditions());
}

int SaveDataElisa_sceSaveDataGetAutoUploadRequestInfo() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetAutoUploadRequestInfo());
}

int SaveDataElisa_sceSaveDataGetAutoUploadSetting() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetAutoUploadSetting());
}

int SaveDataElisa_sceSaveDataGetBoundPsnAccountCount() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetBoundPsnAccountCount());
}

int SaveDataElisa_sceSaveDataGetClientThreadPriority() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetClientThreadPriority());
}

int SaveDataElisa_sceSaveDataGetCloudQuotaInfo() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetCloudQuotaInfo());
}

int SaveDataElisa_sceSaveDataGetDataBaseFilePath() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetDataBaseFilePath());
}

int SaveDataElisa_sceSaveDataGetEventInfo() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetEventInfo());
}

int SaveDataElisa_sceSaveDataGetEventResult(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetEventResult(static_cast<const Libraries::SaveData::OrbisSaveDataEventParam*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataEvent*>(a1)));
}

int SaveDataElisa_sceSaveDataGetFormat() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetFormat());
}

int SaveDataElisa_sceSaveDataGetMountedSaveDataCount() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetMountedSaveDataCount());
}

int SaveDataElisa_sceSaveDataGetMountInfo(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetMountInfo(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataMountInfo*>(a1)));
}

int SaveDataElisa_sceSaveDataGetParam(void* a0, long long a1, void* a2, long long a3, void* a4) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetParam(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataParamType>(a1), static_cast<void*>(a2), static_cast<size_t>(a3), static_cast<size_t*>(a4)));
}

int SaveDataElisa_sceSaveDataGetProgress(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetProgress(static_cast<float*>(a0)));
}

int SaveDataElisa_sceSaveDataGetSaveDataCount() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSaveDataCount());
}

int SaveDataElisa_sceSaveDataGetSaveDataMemory(long long a0, void* a1, long long a2, long long a3) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSaveDataMemory(static_cast<Libraries::UserService::OrbisUserServiceUserId>(a0), static_cast<void*>(a1), static_cast<size_t>(a2), static_cast<int64_t>(a3)));
}

int SaveDataElisa_sceSaveDataGetSaveDataMemory2(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSaveDataMemory2(static_cast<Libraries::SaveData::OrbisSaveDataMemoryGet2*>(a0)));
}

int SaveDataElisa_sceSaveDataGetSaveDataRootDir() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSaveDataRootDir());
}

int SaveDataElisa_sceSaveDataGetSaveDataRootPath() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSaveDataRootPath());
}

int SaveDataElisa_sceSaveDataGetSaveDataRootUsbPath() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSaveDataRootUsbPath());
}

int SaveDataElisa_sceSaveDataGetSavePoint() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetSavePoint());
}

int SaveDataElisa_sceSaveDataGetUpdatedDataCount() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataGetUpdatedDataCount());
}

int SaveDataElisa_sceSaveDataInitialize(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataInitialize(static_cast<void*>(a0)));
}

int SaveDataElisa_sceSaveDataInitialize2(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataInitialize2(static_cast<void*>(a0)));
}

int SaveDataElisa_sceSaveDataInitialize3(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataInitialize3(static_cast<void*>(a0)));
}

int SaveDataElisa_sceSaveDataInitializeForCdlg() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataInitializeForCdlg());
}

int SaveDataElisa_sceSaveDataIsDeletingUsbDb() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataIsDeletingUsbDb());
}

int SaveDataElisa_sceSaveDataIsMounted() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataIsMounted());
}

int SaveDataElisa_sceSaveDataLoadIcon(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataLoadIcon(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataIcon*>(a1)));
}

int SaveDataElisa_sceSaveDataMount(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataMount(static_cast<const Libraries::SaveData::OrbisSaveDataMount*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataMountResult*>(a1)));
}

int SaveDataElisa_sceSaveDataMount2(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataMount2(static_cast<const Libraries::SaveData::OrbisSaveDataMount2*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataMountResult*>(a1)));
}

int SaveDataElisa_sceSaveDataMount5() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataMount5());
}

int SaveDataElisa_sceSaveDataMountInternal() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataMountInternal());
}

int SaveDataElisa_sceSaveDataMountSys() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataMountSys());
}

int SaveDataElisa_sceSaveDataPromote5() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataPromote5());
}

int SaveDataElisa_sceSaveDataRebuildDatabase() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataRebuildDatabase());
}

int SaveDataElisa_sceSaveDataRegisterEventCallback() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataRegisterEventCallback());
}

int SaveDataElisa_sceSaveDataRestoreBackupData(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataRestoreBackupData(static_cast<const Libraries::SaveData::OrbisSaveDataRestoreBackupData*>(a0)));
}

int SaveDataElisa_sceSaveDataRestoreBackupDataForCdlg() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataRestoreBackupDataForCdlg());
}

int SaveDataElisa_sceSaveDataRestoreLoadSaveDataMemory() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataRestoreLoadSaveDataMemory());
}

int SaveDataElisa_sceSaveDataSaveIcon(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSaveIcon(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0), static_cast<const Libraries::SaveData::OrbisSaveDataIcon*>(a1)));
}

int SaveDataElisa_sceSaveDataSetAutoUploadSetting() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetAutoUploadSetting());
}

int SaveDataElisa_sceSaveDataSetEventInfo() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetEventInfo());
}

int SaveDataElisa_sceSaveDataSetParam(void* a0, long long a1, void* a2, long long a3) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetParam(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataParamType>(a1), static_cast<const void*>(a2), static_cast<size_t>(a3)));
}

int SaveDataElisa_sceSaveDataSetSaveDataLibraryUser() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetSaveDataLibraryUser());
}

int SaveDataElisa_sceSaveDataSetSaveDataMemory(long long a0, void* a1, long long a2, long long a3) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetSaveDataMemory(static_cast<Libraries::UserService::OrbisUserServiceUserId>(a0), static_cast<void*>(a1), static_cast<size_t>(a2), static_cast<int64_t>(a3)));
}

int SaveDataElisa_sceSaveDataSetSaveDataMemory2(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetSaveDataMemory2(static_cast<const Libraries::SaveData::OrbisSaveDataMemorySet2*>(a0)));
}

int SaveDataElisa_sceSaveDataSetupSaveDataMemory(long long a0, long long a1, void* a2) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetupSaveDataMemory(static_cast<Libraries::UserService::OrbisUserServiceUserId>(a0), static_cast<size_t>(a1), static_cast<Libraries::SaveData::OrbisSaveDataParam*>(a2)));
}

int SaveDataElisa_sceSaveDataSetupSaveDataMemory2(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetupSaveDataMemory2(static_cast<const Libraries::SaveData::OrbisSaveDataMemorySetup2*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataMemorySetupResult*>(a1)));
}

int SaveDataElisa_sceSaveDataShutdownStart() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataShutdownStart());
}

int SaveDataElisa_sceSaveDataSupportedFakeBrokenStatus() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSupportedFakeBrokenStatus());
}

int SaveDataElisa_sceSaveDataSyncCloudList() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSyncCloudList());
}

int SaveDataElisa_sceSaveDataSyncSaveDataMemory(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataSyncSaveDataMemory(static_cast<Libraries::SaveData::OrbisSaveDataMemorySync*>(a0)));
}

int SaveDataElisa_sceSaveDataTerminate() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataTerminate());
}

int SaveDataElisa_sceSaveDataTransferringMount(void* a0, void* a1) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataTransferringMount(static_cast<const Libraries::SaveData::OrbisSaveDataTransferringMount*>(a0), static_cast<Libraries::SaveData::OrbisSaveDataMountResult*>(a1)));
}

int SaveDataElisa_sceSaveDataUmount(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataUmount(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0)));
}

int SaveDataElisa_sceSaveDataUmountSys() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataUmountSys());
}

int SaveDataElisa_sceSaveDataUmountWithBackup(void* a0) {
    return static_cast<int>(Libraries::SaveData::sceSaveDataUmountWithBackup(static_cast<const Libraries::SaveData::OrbisSaveDataMountPoint*>(a0)));
}

int SaveDataElisa_sceSaveDataUnregisterEventCallback() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataUnregisterEventCallback());
}

int SaveDataElisa_sceSaveDataUpload() {
    return static_cast<int>(Libraries::SaveData::sceSaveDataUpload());
}

int SaveDataElisa_Func_02E4C4D201716422() {
    return static_cast<int>(Libraries::SaveData::Func_02E4C4D201716422());
}

int SaveDataElisa_TestMount2(int user_id, const char* dirname, unsigned int mount_mode,
                             unsigned long long blocks, char* out_mount_point,
                             unsigned int out_mount_point_cap) {
    if (dirname == nullptr || out_mount_point == nullptr || out_mount_point_cap == 0) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataDirName dir{};
    dir.data.FromString(dirname);
    Libraries::SaveData::OrbisSaveDataMount2 mount{};
    mount.userId = static_cast<Libraries::UserService::OrbisUserServiceUserId>(user_id);
    mount.dirName = &dir;
    mount.blocks = static_cast<unsigned long>(blocks);
    mount.mountMode = static_cast<Libraries::SaveData::OrbisSaveDataMountMode>(mount_mode);
    Libraries::SaveData::OrbisSaveDataMountResult result{};
    const int rc = static_cast<int>(Libraries::SaveData::sceSaveDataMount2(&mount, &result));
    if (rc == 0) {
        const auto mp_view = result.mount_point.data.to_view();
        const size_t n = std::min(static_cast<size_t>(out_mount_point_cap - 1), mp_view.size());
        std::memcpy(out_mount_point, mp_view.data(), n);
        out_mount_point[n] = '\0';
    }
    return rc;
}

int SaveDataElisa_TestUmount(const char* mount_point) {
    if (mount_point == nullptr) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataMountPoint mp{};
    mp.data.FromString(mount_point);
    return static_cast<int>(Libraries::SaveData::sceSaveDataUmount(&mp));
}

int SaveDataElisa_TestGetMountInfo(const char* mount_point, unsigned long long* out_blocks,
                                   unsigned long long* out_free_blocks) {
    if (mount_point == nullptr || out_blocks == nullptr || out_free_blocks == nullptr) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataMountPoint mp{};
    mp.data.FromString(mount_point);
    Libraries::SaveData::OrbisSaveDataMountInfo info{};
    const int rc = static_cast<int>(Libraries::SaveData::sceSaveDataGetMountInfo(&mp, &info));
    if (rc == 0) {
        *out_blocks = info.blocks;
        *out_free_blocks = info.freeBlocks;
    }
    return rc;
}

int SaveDataElisa_TestSetParamTitle(const char* mount_point, const char* title) {
    if (mount_point == nullptr || title == nullptr) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataMountPoint mp{};
    mp.data.FromString(mount_point);
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetParam(
        &mp, Libraries::SaveData::OrbisSaveDataParamType::TITLE, title, std::strlen(title) + 1));
}

int SaveDataElisa_TestGetParamTitle(const char* mount_point, char* out_title, unsigned int out_cap,
                                    unsigned long long* out_got_size) {
    if (mount_point == nullptr || out_title == nullptr || out_cap == 0) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataMountPoint mp{};
    mp.data.FromString(mount_point);
    size_t got_size = 0;
    const int rc = static_cast<int>(Libraries::SaveData::sceSaveDataGetParam(
        &mp, Libraries::SaveData::OrbisSaveDataParamType::TITLE, out_title, out_cap, &got_size));
    if (out_got_size != nullptr) {
        *out_got_size = got_size;
    }
    return rc;
}

int SaveDataElisa_TestSetParamUser(const char* mount_point, unsigned int user_param) {
    if (mount_point == nullptr) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataMountPoint mp{};
    mp.data.FromString(mount_point);
    const int value = static_cast<int>(user_param);
    return static_cast<int>(Libraries::SaveData::sceSaveDataSetParam(
        &mp, Libraries::SaveData::OrbisSaveDataParamType::USER_PARAM, &value, sizeof(value)));
}

int SaveDataElisa_TestGetParamUser(const char* mount_point, unsigned int* out_user_param,
                                   unsigned long long* out_got_size) {
    if (mount_point == nullptr || out_user_param == nullptr) {
        return 0x809F0000;
    }
    Libraries::SaveData::OrbisSaveDataMountPoint mp{};
    mp.data.FromString(mount_point);
    size_t got_size = 0;
    unsigned int value = 0;
    const int rc = static_cast<int>(Libraries::SaveData::sceSaveDataGetParam(
        &mp, Libraries::SaveData::OrbisSaveDataParamType::USER_PARAM, &value, sizeof(value),
        &got_size));
    if (rc == 0) {
        *out_user_param = value;
    }
    if (out_got_size != nullptr) {
        *out_got_size = got_size;
    }
    return rc;
}

}
