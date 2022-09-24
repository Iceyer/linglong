/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "job_manager.h"

#include <QDBusConnection>
#include <QTimer>
#include <QUuid>

#include "module/dbus_ipc/dbus_package_manager_common.h"
#include "module/util/job/job.h"

namespace linglong {
namespace package_manager {

class JobManagerPrivate
{
public:
    explicit JobManagerPrivate(JobManager *parent)
        : q_ptr(parent)
    {
    }

    QMap<QString, util::Job *> jobs;

    JobManager *q_ptr = nullptr;
};

JobManager::JobManager()
    : dd_ptr(new JobManagerPrivate(this))
{
}

JobManager::~JobManager() = default;

util::Job *JobManager::createJob(std::function<void(util::Job *)> f, QDBusContext &context)
{
    Q_D(JobManager);

    auto conn = new QDBusConnection(context.connection());

    auto jobId = QUuid::createUuid().toString(QUuid::Id128);
    auto jobPath = QLatin1String(linglong::DBusPackageManagerJobPath) + "/List/" + jobId;
    auto job = new util::Job(jobId, jobPath, f, this);

    d->jobs[jobId] = job;

    connect(job, &util::Job::Finish, this, [=]() {
        qDebug() << "unregisterObject" << job->id() << job;
        // delay unregister object path;
        QTimer::singleShot(5 * 1000, [=]() {
            qDebug() << "conn name" << conn->name();
            conn->unregisterObject(job->path());
            d->jobs.remove(jobId);
            delete conn;
        });
    });

    qDebug() << "registerObject" << job->id() << job << QThread::currentThread();
    conn->registerObject(job->path(), job, QDBusConnection::ExportScriptableContents);

    QTimer::singleShot(50, [=]() {
        job->setProgress(0, "install job start");
        qDebug() << "job startWorker";
        job->startWorker();
    });

    return job;
}

void JobManager::Start(const QString &jobId)
{
}

void JobManager::Stop(const QString &jobId)
{
}

void JobManager::Cancel(const QString &jobId)
{
}

QStringList JobManager::List()
{
    return {};
}

util::Job *JobManager::job(const QString &jobId)
{
    Q_D(JobManager);
    return d->jobs[jobId];
}

} // namespace package_manager
} // namespace linglong
