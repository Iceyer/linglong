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

#include <QDBusArgument>
#include <QList>
#include <QObject>

#include "module/package/package.h"
#include "oci.h"
#include "module/util/result.h"

class Container : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Container)
public:
    Q_SERIALIZE_ITEM_MEMBER(QString, ID, id)
    Q_SERIALIZE_ITEM_MEMBER(qint64, PID, pid)
    Q_SERIALIZE_ITEM_MEMBER(QString, PackageName, packageName)
    Q_SERIALIZE_ITEM_MEMBER(QString, WorkingDirectory, workingDirectory)

    linglong::util::Error create(const QString& ref);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Container)
