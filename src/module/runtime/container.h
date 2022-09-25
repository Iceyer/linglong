/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "module/util/serialize/json.h"
#include "module/util/result.h"

namespace linglong {
namespace runtime {

class Container : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Container)
public:
    Q_SERIALIZE_ITEM_MEMBER(QString, ID, id)
    Q_SERIALIZE_ITEM_MEMBER(qint64, PID, pid)
    Q_SERIALIZE_ITEM_MEMBER(QString, PackageName, packageName)
    Q_SERIALIZE_ITEM_MEMBER(QString, WorkingDirectory, workingDirectory)

    util::Error create(const QString &ref);
};

} // namespace runtime
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::runtime, Container)
