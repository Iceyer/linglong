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

public:
    int progress;
};

Job::Job(std::function<void(Job *)> f, QObject *parent)
    : dd_ptr(new JobPrivate(f, this))
{
    this->connect(dd_ptr->worker, &JobWorker::finish, this, &Job::finish);
}

quint32 Job::Progress() const
{
    Q_D(const Job);
    return d->progress;
}
void Job::setProgress(quint32 progress)
{
    Q_D(Job);
    d->progress = progress;
}

void Job::startWork()
{
    Q_D(Job);
    d->worker->start();
}

Job::~Job() = default;

void JobWorker::run()
{
    auto job = qobject_cast<Job *>(this->parent());
    func(job);
    Q_EMIT this->finish();
}
