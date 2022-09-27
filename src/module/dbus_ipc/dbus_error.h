/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_ERROR_H_
#define LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_ERROR_H_

#include <QString>
#include <QMap>

#include "module/util/status_code.h"

namespace linglong {

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

inline QString dbusErrorName(int code)
{
    auto errorType = static_cast<DBusErrorType>(code);
    return dbusErrorName(errorType);
}

} // namespace linglong

#endif // LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_ERROR_H_
