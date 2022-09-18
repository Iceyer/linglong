/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_PACKAGE_INFO_H_
#define LINGLONG_SRC_MODULE_PACKAGE_INFO_H_

#include <QDBusArgument>
#include <QList>
#include <QObject>

#include "module/util/serialize/json.h"
#include "module/runtime/oci.h"
#include "ref.h"

namespace linglong {
namespace package {

/**
 * @brief User The Info class
 *
 * 包信息类
 */
class User : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(User)
    Q_SERIALIZE_PROPERTY(QString, desktop);
    Q_SERIALIZE_PROPERTY(QString, documents);
    Q_SERIALIZE_PROPERTY(QString, downloads);
    Q_SERIALIZE_PROPERTY(QString, music);
    Q_SERIALIZE_PROPERTY(QString, pictures);
    Q_SERIALIZE_PROPERTY(QString, videos);
    Q_SERIALIZE_PROPERTY(QString, templates);
    Q_SERIALIZE_PROPERTY(QString, temp);
    Q_SERIALIZE_ITEM_MEMBER(QString, public_share, publicShare);
};
/*!
 * \brief The Info class
 * \details 文件系统挂载权限信息
 */
class Filesystem : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Filesystem)
    Q_SERIALIZE_PTR_PROPERTY(User, user);
};

/*!
 * \brief The Info class
 * \details 权限信息类
 */
class Permission : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Permission)
    Q_SERIALIZE_PROPERTY(bool, autostart);
    Q_SERIALIZE_PROPERTY(bool, notification);
    Q_SERIALIZE_PROPERTY(bool, trayicon);
    Q_SERIALIZE_PROPERTY(bool, clipboard);
    Q_SERIALIZE_PROPERTY(bool, account);
    Q_SERIALIZE_PROPERTY(bool, bluetooth);
    Q_SERIALIZE_PROPERTY(bool, camera);
    Q_SERIALIZE_ITEM_MEMBER(bool, audio_record, audioRecord);
    Q_SERIALIZE_ITEM_MEMBER(bool, installed_apps, installedApps);
    Q_SERIALIZE_PTR_PROPERTY(Filesystem, filesystem);
};

class OverlayfsRootfs : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(OverlayfsRootfs)
    Q_SERIALIZE_PROPERTY(MountList, mounts);
};

/*!
 * Info is the data of /opt/apps/{package-id}/info.json. The spec can get from here:
 * https://doc.chinauos.com/content/M7kCi3QB_uwzIp6HyF5J
 */
class Info : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Info)

public:
    Q_SERIALIZE_PROPERTY(QString, appid);
    Q_SERIALIZE_PROPERTY(QString, version);
    Q_SERIALIZE_PROPERTY(QStringList, arch);
    Q_SERIALIZE_PROPERTY(QString, kind);
    Q_SERIALIZE_PROPERTY(QString, name);
    Q_SERIALIZE_PROPERTY(QString, module);
    Q_SERIALIZE_PROPERTY(quint64, size);
    Q_SERIALIZE_PROPERTY(QString, description);

    // ref of runtime
    Q_SERIALIZE_PROPERTY(QString, runtime);
    Q_SERIALIZE_PROPERTY(QString, base);

    // permissions
    Q_SERIALIZE_PTR_PROPERTY(Permission, permissions);

    //overlayfs mount
    Q_SERIALIZE_PTR_PROPERTY(OverlayfsRootfs, overlayfs);

};

} // namespace package
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::package, Info)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::package, Permission)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::package, Filesystem)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::package, User)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::package, OverlayfsRootfs)

// inline QDBusArgument &operator<<(QDBusArgument &argument, const linglong::package::Info &message)
//{
//     argument.beginStructure();
//     argument << message.appid;
//     argument << message.name;
//     argument.endStructure();
//     return argument;
// }
//
// inline const QDBusArgument &operator>>(const QDBusArgument &argument, linglong::package::Info &message)
//{
//     argument.beginStructure();
//     argument >> message.appid;
//     argument >> message.name;
//     argument.endStructure();
//     return argument;
// }

#endif /* LINGLONG_SRC_MODULE_PACKAGE_INFO_H_ */
