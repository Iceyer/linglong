/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QString>

namespace linglong {
namespace util {

QString hostArch();

QString getUserName(uid_t uid);

QString getUserName();

QString getUserHomePath(uid_t uid);

} // namespace util
} // namespace linglong
