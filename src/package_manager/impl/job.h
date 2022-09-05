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
#include <QThread>
#include <QDBusContext>

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
    void finish();

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

    // DBus export contents
public Q_SLOTS:
    Q_SCRIPTABLE quint32 Progress() const;
Q_SIGNALS:
    Q_SCRIPTABLE void ProgressChange(quint32 progress);

public:
    QString id;
    Job(std::function<void(Job *)> f, QObject *parent);
    ~Job();

    void startWork();
    void setProgress(quint32 progress);

Q_SIGNALS:
    void finish();

private:
    QScopedPointer<JobPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), Job)
};
