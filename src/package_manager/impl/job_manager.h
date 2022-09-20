/*
 * Copyright (c) 2020. Uniontech Software Ltd. All rights reserved.
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

#include "module/util/singleton.h"
#include "module/runtime/container.h"

namespace linglong {

namespace util {
class Job;
}

namespace package_manager {

class JobManagerPrivate;
class JobManager
    : public QObject
    , protected QDBusContext
    , public linglong::util::Singleton<JobManager>
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.linglong.JobManager")
public Q_SLOTS:
    Q_SCRIPTABLE QStringList List();
    Q_SCRIPTABLE void Start(const QString &jobId);
    Q_SCRIPTABLE void Stop(const QString &jobId);
    Q_SCRIPTABLE void Cancel(const QString &jobId);

public:
    util::Job *createJob(std::function<void(util::Job *)> f, QDBusConnection *conn);
    util::Job *job(const QString &jobId);

protected:
    JobManager();
    ~JobManager() override;

private:
    friend class linglong::util::Singleton<JobManager>;
    QScopedPointer<JobManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), JobManager)
};

} // namespace package_manager
} // namespace linglong
