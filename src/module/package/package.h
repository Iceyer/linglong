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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string>
#include <QDBusArgument>
#include <QObject>
#include <QList>
#include <QFile>
#include <QDebug>
#include <QTemporaryDir>
#include <QFileInfo>

#include "module/util/file.h"
#include "module/util/serialize/json.h"
#include "module/util/status_code.h"
#include "ref.h"

// 当前在线包对应的包名/版本/架构/uab存储URL信息
namespace linglong {
namespace package {

class MetaInfo : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(MetaInfo)

public:
    Q_SERIALIZE_PROPERTY(QString, appId);
    Q_SERIALIZE_PROPERTY(QString, name);
    Q_SERIALIZE_PROPERTY(QString, version);
    Q_SERIALIZE_PROPERTY(QString, arch);
    // app 类型
    Q_SERIALIZE_PROPERTY(QString, kind);
    Q_SERIALIZE_PROPERTY(QString, runtime);
    // 软件包对应的uab存储地址
    Q_SERIALIZE_PROPERTY(QString, uabUrl);
    // app 所属远端仓库名称
    Q_SERIALIZE_PROPERTY(QString, repoName);
    // app 描述
    Q_SERIALIZE_PROPERTY(QString, description);
    // 安装应用对应的用户
    Q_SERIALIZE_PROPERTY(QString, user);

    // app 大小
    Q_SERIALIZE_PROPERTY(QString, size);
    // app 渠道
    Q_SERIALIZE_PROPERTY(QString, channel);
    // app 版本类型 devel/release
    Q_SERIALIZE_PROPERTY(QString, module);

public:
    inline package::Ref ref() { return package::Ref("", channel, appId, version, arch, module); }
};

} // namespace package
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::package, MetaInfo)

typedef QStringMap QStringMap;

namespace linglong {
namespace package {
void registerAllMetaType();
} // namespace package
} // namespace linglong