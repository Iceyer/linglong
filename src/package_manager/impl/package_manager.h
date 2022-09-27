/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     HuQinghong <huqinghong@uniontech.com>
 *
 * Maintainer: HuQinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDBusArgument>
#include <QDBusContext>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include "module/dbus_ipc/package_manager_param.h"
#include "module/util/singleton.h"
#include "module/util/status_code.h"
#include "module/dbus_ipc/reply.h"
#include "module/dbus_ipc/param_option.h"
#include "module/package/package.h"
#include "package_manager_flatpak_impl.h"
#include "module/dbus_ipc/register_meta_type.h"
#include "module/runtime/container.h"

namespace linglong {
namespace package_manager {

class PackageManagerPrivate;

class PackageManager
    : public QObject
    , protected QDBusContext
    , public linglong::util::Singleton<PackageManager>
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.linglong.PackageManager")

public Q_SLOTS:
    Reply ModifyRepo(const QString &url);

    QString Install(const QString &ref, const QVariantMap &options);
    QString Update(const QString &ref, const QVariantMap &options);
    QString Uninstall(const QString &ref, const QVariantMap &options);
    QVariantMap Show(const QString &ref, const QVariantMap &options);
    QVariantMapList Query(const QString &ref, const QVariantMap &options);
    QVariantMapList List(const QVariantMap &options);

protected:
    PackageManager();
    ~PackageManager() override;

private:
    friend class linglong::util::Singleton<PackageManager>;

    QScopedPointer<PackageManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), PackageManager)
};

} // namespace package_manager
} // namespace linglong
