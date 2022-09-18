/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "package.h"

#include <QMetaType>

#include "info.h"

void linglong::package::registerAllMetaType()
{
    qSerializeRegister<linglong::package::Info>();
    qSerializeRegister<linglong::package::Permission>();
    qSerializeRegister<linglong::package::Filesystem>();
    qSerializeRegister<linglong::package::User>();
    qSerializeRegister<linglong::package::OverlayfsRootfs>();
    qSerializeRegister<linglong::package::AppMetaInfo>();
}
