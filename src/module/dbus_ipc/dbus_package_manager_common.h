/*
 * Copyright (c) 2022. ${ORGANIZATION_NAME}. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_PACKAGE_MANAGER_COMMON_H_
#define LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_PACKAGE_MANAGER_COMMON_H_

namespace linglong {

const char *const DBusPackageManagerServiceName = "org.deepin.linglong.PackageManager";

const char *const DBusPackageManagerPath = "/org/deepin/linglong/PackageManager";
const char *const DBusPackageManagerInterface = "org.deepin.linglong.PackageManager";

const char *const DBusPackageManagerJobManagerPath = "/org/deepin/linglong/JobManager";
const char *const DBusPackageManagerJobManagerInterface = "org.deepin.linglong.JobManager";

const char *const DBusPackageManagerJobPath = "/org/deepin/linglong/Job";
const char *const DBusPackageManagerJobInterface = "org.deepin.linglong.Job";

} // namespace linglong

#endif // LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_PACKAGE_MANAGER_COMMON_H_
