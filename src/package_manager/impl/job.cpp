/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "job.h"
#include "module/util/status_code.h"
#include <qdebug.h>

namespace linglong {
namespace package_manager {

class JobPrivate
{
public:
    explicit JobPrivate(std::function<void(Job *)> f, Job *parent)
        : worker(new JobWorker(f, parent))
        , q_ptr(parent)
    {
    }

    JobWorker *worker = nullptr;
    Job *q_ptr = nullptr;
    QString id;
    QString path;

    quint32 progress = 0;
    quint64 rate = 0;
    quint64 averageRate = 0;
    qint64 remaining = -1;
    QString message;
    quint32 finishCode = quint32(linglong::util::StatusCode::kPkgUpdating);
};

Job::Job(const QString &id, const QString &path, std::function<void(Job *)> f, QObject *parent)
    : dd_ptr(new JobPrivate(f, this))
{
    Q_D(Job);
    d->id = id;
    d->path = path;
    this->connect(dd_ptr->worker, &JobWorker::finish, this, &Job::Finish);
    this->connect(dd_ptr->worker, &JobWorker::progressChanged, this, &Job::ProgressChanged);
}

void Job::setProgress(quint32 progress, const QString &message)
{
    Q_D(Job);
    d->progress = progress;
    d->message = message;

    Q_EMIT d->worker->progressChanged(progress, d->rate, d->averageRate, d->remaining, message);
}

void Job::setProgress(quint32 progress, qint64 remaining, const QString &message)
{
    Q_D(Job);
    d->progress = progress;
    d->message = message;
    d->remaining = remaining;

    Q_EMIT d->worker->progressChanged(progress, d->rate, d->averageRate, d->remaining, message);
}

void Job::setProgress(quint32 progress, quint64 rate, quint64 averageRate, qint64 remaining, const QString &message)
{
    Q_D(Job);
    d->progress = progress;
    d->remaining = remaining;
    d->message = message;
    d->rate = rate;
    d->averageRate = averageRate;

    Q_EMIT d->worker->progressChanged(progress, rate, averageRate, remaining, message);
}

/*!
 * finish job with an error code and message
 * @param code
 * @param message
 */
void Job::setFinish(quint32 code, const QString &message)
{
    Q_D(Job);
    d->finishCode = code;
    qDebug() << "------" << code;
    Q_EMIT d->worker->finish(code, message);
}

void Job::startWorker()
{
    Q_D(Job);
    d->worker->start();
}

quint32 Job::progress() const
{
    Q_D(const Job);
    return d->progress;
}

quint64 Job::rate() const
{
    Q_D(const Job);
    return d->rate;
}

quint64 Job::averageRate() const
{
    Q_D(const Job);
    return d->averageRate;
}

qint64 Job::remaining() const
{
    Q_D(const Job);
    return d->remaining;
}

QString Job::message() const
{
    Q_D(const Job);
    return d->message;
}

QString Job::path() const
{
    Q_D(const Job);
    return d->path;
}

QString Job::id() const
{
    Q_D(const Job);
    return d->id;
}

quint32 Job::finishCode() const
{
    Q_D(const Job);
    return d->finishCode;
}

Job::~Job() = default;

void JobWorker::run()
{
    auto job = qobject_cast<Job *>(this->parent());
    func(job);
}

} // namespace package_manager
} // namespace linglong
