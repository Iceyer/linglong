/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_UTIL_JOB_JOB_CONTROLLER_H_
#define LINGLONG_SRC_MODULE_UTIL_JOB_JOB_CONTROLLER_H_

#include <QScopedPointer>
#include <QThread>
#include <QMutex>

namespace linglong {
namespace util {
class Job;
class JobPrivate;

enum JobState {
    JobStateRunning = 0x0001,
    JobStatePause,
    JobStateFinish,
};

class JobController : public QObject
{
    Q_OBJECT
public:
    JobController(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    JobState waitForStatus();

Q_SIGNALS:
    void progressChanged(const QVariantMap &extraData);

private:
    void pause();
    void resume();
    void cancel();
    JobState state();

    // FIXME: no safe to export mutex and status, move to JobControllerPrivate
    friend linglong::util::Job;
    friend linglong::util::JobPrivate;

    JobState m_status = JobStateRunning;
    QMutex runningMutex;
};

} // namespace util
} // namespace linglong

#endif // LINGLONG_SRC_MODULE_UTIL_JOB_JOB_CONTROLLER_H_
