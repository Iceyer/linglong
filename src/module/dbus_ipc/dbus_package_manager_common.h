/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_PACKAGE_MANAGER_COMMON_H_
#define LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_PACKAGE_MANAGER_COMMON_H_

#include "dbus_common.h"
#include "module/util/status_code.h"

namespace linglong {

const char *const DBusPackageManagerServiceName = "org.deepin.linglong.PackageManager";

const char *const DBusPackageManagerPath = "/org/deepin/linglong/PackageManager";
const char *const DBusPackageManagerInterface = "org.deepin.linglong.PackageManager";

const char *const DBusPackageManagerJobManagerPath = "/org/deepin/linglong/JobManager";
const char *const DBusPackageManagerJobManagerInterface = "org.deepin.linglong.JobManager";

const char *const DBusPackageManagerJobPath = "/org/deepin/linglong/Job";
const char *const DBusPackageManagerJobInterface = "org.deepin.linglong.Job";

// DBus error type
enum class DBusErrorType {
    Success = 0,

    PackageNotInstalled = static_cast<int>(util::StatusCode::PackageNotInstalled),

    UnknownError = -1,
};

inline QString dbusErrorName(const DBusErrorType &errorType)
{
    static const QMap<DBusErrorType, QString> errorNameMap = {
        {DBusErrorType::PackageNotInstalled, "org.deepin.linglong.DBus.Error.PackageNotInstalled"},
        {DBusErrorType::UnknownError, "org.deepin.linglong.DBus.Error.UnknownError"},
    };

    if (errorNameMap.contains(errorType)) {
        return errorNameMap[errorType];
    } else {
        return errorNameMap[DBusErrorType::UnknownError];
    }
}

inline QString dbusErrorName(const util::StatusCode &statusCode)
{
    auto errorType = static_cast<DBusErrorType>(statusCode);
    return dbusErrorName(errorType);
}

} // namespace linglong

#endif // LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_PACKAGE_MANAGER_COMMON_H_
