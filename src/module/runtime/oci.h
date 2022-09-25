/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <mutex>
#include "module/util/serialize/json.h"

class Root : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Root)
public:
    Q_SERIALIZE_PROPERTY(QString, path);
    Q_SERIALIZE_PROPERTY(bool, readonly);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Root)

class Namespace : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Namespace)
    Q_SERIALIZE_PROPERTY(QString, type);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Namespace)

class IdMap : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(IdMap)
    Q_SERIALIZE_ITEM_MEMBER(quint64, hostID, hostId);
    Q_SERIALIZE_ITEM_MEMBER(quint64, containerID, containerId);
    Q_SERIALIZE_PROPERTY(quint64, size);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(IdMap)

class Linux : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Linux)
    Q_SERIALIZE_PROPERTY(NamespaceList, namespaces);
    Q_SERIALIZE_PROPERTY(IdMapList, uidMappings);
    Q_SERIALIZE_PROPERTY(IdMapList, gidMappings);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Linux)

class Process : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Process)
    Q_SERIALIZE_PROPERTY(QStringList, args);
    Q_SERIALIZE_PROPERTY(QStringList, env);
    Q_SERIALIZE_PROPERTY(QString, cwd);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Process)

class Mount : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Mount)
    Q_SERIALIZE_PROPERTY(QString, destination);
    Q_SERIALIZE_PROPERTY(QString, type);
    Q_SERIALIZE_PROPERTY(QString, source);
    Q_SERIALIZE_PROPERTY(QStringList, options);

private:
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Mount)

class Hook : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Hook)
    Q_SERIALIZE_PROPERTY(QString, path);
    Q_SERIALIZE_PROPERTY(QStringList, args);
    Q_SERIALIZE_PROPERTY(QStringList, env);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Hook)

class Hooks : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Hooks)
    Q_SERIALIZE_PROPERTY(HookList, prestart);
    Q_SERIALIZE_PROPERTY(HookList, poststart);
    Q_SERIALIZE_PROPERTY(HookList, poststop);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Hooks)

class AnnotationsOverlayfsRootfs : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(AnnotationsOverlayfsRootfs)
    Q_SERIALIZE_PROPERTY(QString, lowerParent);
    Q_SERIALIZE_PROPERTY(QString, upper);
    Q_SERIALIZE_PROPERTY(QString, workdir);
    Q_SERIALIZE_PROPERTY(MountList, mounts);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(AnnotationsOverlayfsRootfs)

class AnnotationsNativeRootfs : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(AnnotationsNativeRootfs)
    Q_SERIALIZE_PROPERTY(MountList, mounts);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(AnnotationsNativeRootfs)

class DBusProxy : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(DBusProxy)
    Q_SERIALIZE_PROPERTY(bool, enable);
    Q_SERIALIZE_PROPERTY(QString, busType);
    Q_SERIALIZE_ITEM_MEMBER(QString, appID, appId);
    Q_SERIALIZE_PROPERTY(QString, proxyPath);
    Q_SERIALIZE_PROPERTY(QStringList, name);
    Q_SERIALIZE_PROPERTY(QStringList, path);
    Q_SERIALIZE_PROPERTY(QStringList, interface);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(DBusProxy)

class Annotations : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Annotations)
    Q_SERIALIZE_PROPERTY(QString, containerRootPath);
    Q_SERIALIZE_PTR_PROPERTY(AnnotationsOverlayfsRootfs, overlayfs);
    Q_SERIALIZE_PTR_PROPERTY(AnnotationsNativeRootfs, native);

    Q_SERIALIZE_PTR_PROPERTY(DBusProxy, dbusProxyInfo);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Annotations)

#undef linux
class Runtime : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Runtime)
    Q_SERIALIZE_PROPERTY(QString, ociVersion);
    Q_SERIALIZE_PTR_PROPERTY(Root, root);
    Q_SERIALIZE_PTR_PROPERTY(Process, process);
    Q_SERIALIZE_PROPERTY(QString, hostname);
    Q_SERIALIZE_PROPERTY(MountList, mounts);
    Q_SERIALIZE_PTR_PROPERTY(Linux, linux);
    Q_SERIALIZE_PTR_PROPERTY(Hooks, hooks);
    Q_SERIALIZE_PTR_PROPERTY(Annotations, annotations);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(Runtime)

namespace linglong {
namespace runtime {

inline void registerAllOciMetaType()
{
    static std::once_flag flag;
    std::call_once(flag, []() {
        qSerializeRegister<Root>();
        qSerializeRegister<Linux>();
        qSerializeRegister<Mount>();
        qSerializeRegister<Namespace>();
        qSerializeRegister<Hook>();
        qSerializeRegister<Runtime>();
        qSerializeRegister<Process>();
        qSerializeRegister<IdMap>();
        qSerializeRegister<DBusProxy>();
        qSerializeRegister<Annotations>();
        qSerializeRegister<AnnotationsOverlayfsRootfs>();
        qSerializeRegister<AnnotationsNativeRootfs>();
    });
}

} // namespace runtime
} // namespace linglong
