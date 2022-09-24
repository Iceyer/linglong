/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
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

#include "register_meta_type.h"
#include "module/package/package.h"
#include "module/runtime/container.h"
#include "module/util/package_manager_param.h"
#include "module/util/singleton.h"
#include "module/util/status_code.h"
#include "reply.h"
#include "param_option.h"

namespace linglong {
namespace service {

class AppManagerPrivate; /**< forward declaration PackageManagerPrivate */

/**
 * @brief The PackageManager class
 * @details PackageManager is a singleton class, and it is used to manage the package
 *          information.
 */
class AppManager
    : public QObject
    , protected QDBusContext
    , public linglong::util::Singleton<AppManager>
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.linglong.AppManager")
public Q_SLOTS:
    QString Status();

    /**
     * @brief 运行应用
     *
     * @param paramOption 启动命令参数
     *
     * @return Reply 同Install
     */
    Reply Start(const RunParamOption &paramOption);

    /**
     * @brief 运行命令
     *
     * @param paramOption 启动命令参数
     *
     * @return Reply 同 Install
     */
    Reply Exec(const ExecParamOption &paramOption);

    /**
     * @brief 退出应用
     *
     * @param containerId 运行应用容器对应的Id（使用ListContainer查询）
     *
     * @return Reply 执行结果信息
     */
    Reply Stop(const QString &containerId);

    /**
     * @brief 查询正在运行的应用信息
     *
     * @return ContainerList \n
     *         Id 容器id \n
     *         pid 容器对应的进程id \n
     *         packageName 应用名称 \n
     *         workingDirectory 应用运行目录
     */
    QueryReply List();

    /**
     * @brief 执行终端命令
     *
     * @param exe 命令
     * @param args 参数
     *
     * @return Reply 执行结果信息
     */
    Reply RunCommand(const QString &exe, const QStringList args);

public:
    // FIXME: DO NOT expose inter details
    QThreadPool &pool();

protected:
    AppManager();
    ~AppManager() override;

private:
    friend class linglong::util::Singleton<AppManager>;
    QScopedPointer<AppManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), AppManager)
};

} // namespace service
} // namespace linglong

#define RUN_POOL_MAX_THREAD 100000000 ///< 启动应用线程池最大线程数
#define APP_MANAGER linglong::service::AppManager::instance()
