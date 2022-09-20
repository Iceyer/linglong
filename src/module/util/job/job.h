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

#include <QScopedPointer>
#include <QDBusContext>
#include <QThread>

#include "module/util/job/job_controller.h"

namespace linglong {
namespace util {
class Job;

class JobWorker : public QThread
{
    Q_OBJECT
public:
    JobWorker(std::function<void(Job *)> f, QObject *parent)
        : QThread(parent)
        , func(f)
    {
    }

    void run() override;

Q_SIGNALS:
    void progressChanged(quint32 progress, quint64 rate, quint64 averageRate, qint64 remaining, const QString &message);
    void finish(quint32 code, const QString &message);

private:
    std::function<void(Job *)> func;
};

class JobPrivate;
class Job
    : public QObject
    , protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.linglong.Job")

    Q_PROPERTY(quint32 Progress READ progress)
    Q_PROPERTY(quint64 Rate READ rate)
    Q_PROPERTY(quint64 AverageRate READ averageRate)
    Q_PROPERTY(qint64 Remaining READ remaining)
    Q_PROPERTY(QString Message READ message)
    Q_PROPERTY(quint32 State READ state)
    Q_PROPERTY(quint32 StatusCode READ statusCode)

public Q_SLOTS:
    Q_SCRIPTABLE void Pause();
    Q_SCRIPTABLE void Resume();
    Q_SCRIPTABLE void Cancel();
Q_SIGNALS:
    Q_SCRIPTABLE void ProgressChanged(quint32 progress, quint64 rate, quint64 averageRate, qint64 remaining,
                                      const QString &message);
    Q_SCRIPTABLE void Finish(quint32 code, const QString &message);

public:
    quint32 progress() const;
    quint64 rate() const;
    quint64 averageRate() const;
    qint64 remaining() const;
    QString message() const;
    quint32 state() const;
    quint32 statusCode() const;

public:
    Job(const QString &id, const QString &path, std::function<void(Job *)> f, QObject *parent);
    ~Job();

    void startWorker();

    util::JobController *controller();

    QString id() const;
    QString path() const;

    void setProgress(quint32 progress, const QString &message);
    void setProgress(quint32 progress, qint64 remaining, const QString &message);
    void setProgress(quint32 progress, quint64 rate, quint64 averageRate, qint64 remaining, const QString &message);

    void setFinish(quint32 code, const QString &message);

private:
    friend class JobWorker;
    QScopedPointer<JobPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), Job)
};

} // namespace package_manager
} // namespace linglong
