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

#include <QDebug>

namespace linglong {
namespace util {

class JobPrivate
{
public:
    explicit JobPrivate(std::function<void(Job *)> f, Job *parent)
        : worker(new JobWorker(f, parent))
        , controller(new util::JobController(parent))
        , q_ptr(parent)
    {
    }

    JobWorker *worker = nullptr;
    util::JobController *controller = nullptr;
    QMutex runningMutex;

    Job *q_ptr = nullptr;

    QString id;
    QString path;

    quint32 progress = 0;
    quint64 rate = 0;
    quint64 averageRate = 0;
    qint64 remaining = -1;
    QString message;
    quint32 statusCode = 0;
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
    d->statusCode = code;
    d->message = message;
    // todo: add a finish method to controller?
    d->controller->cancel();
    Q_EMIT d->worker->finish(d->statusCode, d->message);
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

void Job::Pause()
{
    Q_D(Job);
    d->controller->pause();
}

void Job::Resume()
{
    Q_D(Job);
    d->controller->resume();
}

quint32 Job::statusCode() const
{
    Q_D(const Job);
    return d->statusCode;
}

void Job::Cancel()
{
    Q_D(Job);
    d->controller->cancel();
}

util::JobController *Job::controller()
{
    Q_D(Job);
    return d->controller;
}

quint32 Job::state() const
{
    Q_D(const Job);
    return static_cast<quint32>(d->controller->state());
}

Job::~Job() = default;

void JobWorker::run()
{
    auto job = qobject_cast<Job *>(this->parent());
    func(job);
}

} // namespace util
} // namespace linglong
