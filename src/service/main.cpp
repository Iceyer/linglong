/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>

#include "dbus_gen_app_manager_adaptor.h"
#include "module/dbus_ipc/dbus_app_manager_common.h"
#include "module/runtime/app.h"
#include "module/runtime/runtime.h"
#include "module/util/log_handler.h"

#include "impl/register_meta_type.h"

using namespace linglong;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("deepin");

    // 安装消息处理函数
    LOG_HANDLER->installMessageHandler();

    linglong::runtime::registerAllMetaType();
    linglong::package::registerAllMetaType();
    linglong::service::registerAllMetaType();

    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.registerService(AppManagerDBusServiceName)) {
        qCritical() << "service exist" << dbus.lastError();
        return -1;
    }

    AppManagerAdaptor appManagerAdaptor(service::AppManager::instance());

    if (!dbus.registerObject(AppManagerDBusPath, service::AppManager::instance())) {
        qCritical() << "path exist" << AppManagerDBusPath << dbus.lastError();
        return -1;
    }

    app.connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
        dbus.unregisterObject(AppManagerDBusPath);
        dbus.unregisterService(AppManagerDBusServiceName);
    });

    return QCoreApplication::exec();
}
