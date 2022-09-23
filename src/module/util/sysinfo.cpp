/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sysinfo.h"

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <QDebug>
#include <QSysInfo>

namespace linglong {
namespace util {

QString hostArch()
{
    // https://doc.qt.io/qt-5/qsysinfo.html#currentCpuArchitecture
    // "arm64"
    // "mips64"
    // "x86_64"
    QString arch = QSysInfo::currentCpuArchitecture();
    // FIXME: the support arch should be list. and how to extern an new arch?
    return arch;
}

QString getUserName(uid_t uid)
{
    struct passwd *user = getpwuid(uid);
    if (user) {
        return QString::fromUtf8(user->pw_name);
    }
    qCritical() << "getUserName err";
    return "";
}

QString getUserHomePath(uid_t uid)
{
    struct passwd *user = getpwuid(uid);
    if (user) {
        return QString::fromUtf8(user->pw_dir);
    }
    qCritical() << "getUserHomePath filed";
    return "";
}

/*
 * 查询当前登陆用户名
 *
 * @return QString: 当前登陆用户名
 */
QString getUserName()
{
    return getUserName(geteuid());
}
} // namespace util
} // namespace linglong
