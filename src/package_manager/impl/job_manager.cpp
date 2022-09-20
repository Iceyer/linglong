/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "module/repo/ostree_repohelper.h"
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

util::Job *JobManager::createJob(std::function<void(util::Job *)> f, QDBusConnection *conn)
{
    Q_D(JobManager);
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
    if (jobId.isNull() || jobId.isEmpty()) {
        qCritical() << "jobID err";
        return;
    }

    int processId = OSTREE_REPO_HELPER->getOstreeJobId(jobId);
    if (processId == -1) {
        qWarning() << jobId << " not exist";
        return;
    }
    qInfo() << "restart job:" << jobId << ", process:" << processId;
    QProcess process;
    QStringList argStrList = {"-CONT", QString::number(processId)};
    process.start("kill", argStrList);
    if (!process.waitForStarted()) {
        qCritical() << "kill -CONT failed!";
        return;
    }
    if (!process.waitForFinished(3000)) {
        qCritical() << "run finish failed!";
        return;
    }

    auto retStatus = process.exitStatus();
    auto retCode = process.exitCode();
    if (retStatus != 0 || retCode != 0) {
        qCritical() << " run failed, retCode:" << retCode
                    << ", info msg:" << QString::fromLocal8Bit(process.readAllStandardOutput())
                    << ", err msg:" << QString::fromLocal8Bit(process.readAllStandardError());
    }
}

// 下载应用的时候 正在下载runtime 如何停止？
void JobManager::Stop(const QString &jobId)
{
    if (jobId.isNull() || jobId.isEmpty()) {
        qCritical() << "jobId err";
        return;
    }

    int processId = OSTREE_REPO_HELPER->getOstreeJobId(jobId);
    if (processId == -1) {
        qWarning() << jobId << " not exist";
        return;
    }
    qInfo() << "pause job:" << jobId << ", process:" << processId;
    QProcess process;
    QStringList argStrList = {"-STOP", QString::number(processId)};
    process.start("kill", argStrList);
    if (!process.waitForStarted()) {
        qCritical() << "kill -STOP failed!";
        return;
    }
    if (!process.waitForFinished(3000)) {
        qCritical() << "run finish failed!";
        return;
    }

    auto retStatus = process.exitStatus();
    auto retCode = process.exitCode();
    if (retStatus != 0 || retCode != 0) {
        qCritical() << " run failed, retCode:" << retCode
                    << ", info msg:" << QString::fromLocal8Bit(process.readAllStandardOutput())
                    << ", err msg:" << QString::fromLocal8Bit(process.readAllStandardError());
    }
}

// Fix to do 取消之后再下载问题
void JobManager::Cancel(const QString &jobId)
{
    if (jobId.isNull() || jobId.isEmpty()) {
        qCritical() << "jobId err";
        return;
    }

    int processId = OSTREE_REPO_HELPER->getOstreeJobId(jobId);
    if (processId == -1) {
        qWarning() << jobId << " not exist";
        return;
    }

    qInfo() << "cancel job:" << jobId << ", process:" << processId;
    QProcess process;
    QStringList argStrList = {"-9", QString::number(processId)};
    process.start("kill", argStrList);
    if (!process.waitForStarted()) {
        qCritical() << "kill failed!";
        return;
    }
    if (!process.waitForFinished(3000)) {
        qCritical() << "run finish failed!";
        return;
    }

    auto retStatus = process.exitStatus();
    auto retCode = process.exitCode();
    if (retStatus != 0 || retCode != 0) {
        qCritical() << " run failed, retCode:" << retCode
                    << ", info msg:" << QString::fromLocal8Bit(process.readAllStandardOutput())
                    << ", err msg:" << QString::fromLocal8Bit(process.readAllStandardError());
    }
}

QStringList JobManager::List()
{
    return OSTREE_REPO_HELPER->getOstreeJobList();
}

util::Job *JobManager::job(const QString &jobId)
{
    Q_D(JobManager);
    return d->jobs[jobId];
}

} // namespace package_manager
} // namespace linglong
