/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "job_controller.h"

namespace linglong {
namespace util {

void JobController::pause()
{
    if (m_status != JobStateRunning) {
        return;
    }
    runningMutex.lock();
    m_status = JobStatePause;
}

void JobController::resume()
{
    m_status = JobStateRunning;
    runningMutex.unlock();
}

void JobController::cancel()
{
    runningMutex.unlock();
    m_status = JobStateFinish;
}

JobState JobController::waitForStatus()
{
    runningMutex.lock();
    runningMutex.unlock();
    return m_status;
}

JobState JobController::state()
{
    return m_status;
}

} // namespace util
} // namespace linglong