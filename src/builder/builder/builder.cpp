/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "builder.h"

#include "project.h"

void linglong::builder::registerAllMetaType()
{
    qSerializeRegister<linglong::builder::BuildDepend>();
    qSerializeRegister<linglong::builder::Project>();
    qSerializeRegister<linglong::builder::Variables>();
    qSerializeRegister<linglong::builder::Package>();
    qSerializeRegister<linglong::builder::BuilderRuntime>();
    qSerializeRegister<linglong::builder::Source>();
    qSerializeRegister<linglong::builder::Build>();
    qSerializeRegister<linglong::builder::BuildManual>();
}
