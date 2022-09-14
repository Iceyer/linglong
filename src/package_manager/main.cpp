/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     HuQinghong <huqinghong@uniontech.com>
 *
 * Maintainer: HuQinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>

#include "service/impl/register_meta_type.h"
#include "module/util/log_handler.h"
#include "module/repo/repo.h"
#include "module/dbus_ipc/dbus_package_manager_common.h"

#include "dbus_gen_package_manager_adaptor.h"
#include "dbus_gen_job_manager_adaptor.h"

using namespace linglong;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("deepin");
    // 安装消息处理函数
    LOG_HANDLER->installMessageHandler();

    linglong::package::registerAllMetaType();
    linglong::service::registerAllMetaType();
    linglong::repo::registerAllMetaType();

    QDBusConnection dbus = QDBusConnection::systemBus();
    if (!dbus.registerService(DBusPackageManagerServiceName)) {
        qCritical() << "service exist" << dbus.lastError();
        return -1;
    }

    PackageManagerAdaptor packageManagerAdaptor(package_manager::SystemPackageManager::instance());
    JobManagerAdaptor jma(package_manager::JobManager::instance());

    dbus.registerObject(DBusPackageManagerPath, package_manager::SystemPackageManager::instance());
    dbus.registerObject(DBusPackageManagerJobManagerPath, package_manager::JobManager::instance());

    return QCoreApplication::exec();
}
