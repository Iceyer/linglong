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

#include "module/util/serialize/yaml.h"
#include "src/module/runtime/oci.h"

namespace linglong {
namespace test {

class MountRule : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(MountRule)
    Q_SERIALIZE_PROPERTY(QString, type);
    Q_SERIALIZE_PROPERTY(QString, options);
    Q_SERIALIZE_PROPERTY(QString, source);
    Q_SERIALIZE_PROPERTY(QString, destination);
};
Q_SERIALIZE_DECLARE_TYPE(MountRule)

class Permission : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Permission)
    // NOTE: custom class type must use with full namespace in qt type system
    Q_SERIALIZE_PROPERTY(linglong::test::MountRuleList, mounts);
};
Q_SERIALIZE_DECLARE_TYPE(Permission)

class App : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(App)
    Q_SERIALIZE_PROPERTY(QString, version);
    // NOTE: custom class type must use with full namespace in qt type system
    Q_SERIALIZE_PTR_PROPERTY(linglong::test::Permission, permissions);
};
Q_SERIALIZE_DECLARE_TYPE(App)

} // namespace test
} // namespace linglong

Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::test, MountRule)
Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::test, Permission)
Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::test, App)

class TestMount : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(TestMount)
    Q_SERIALIZE_PROPERTY(QString, type);
    Q_SERIALIZE_PROPERTY(QString, source);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(TestMount)

class TestPermission : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(TestPermission)
    Q_SERIALIZE_PROPERTY(TestMountList, mounts);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(TestPermission)

class TestApp : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(TestApp)
    Q_SERIALIZE_PROPERTY(QString, version);
    Q_SERIALIZE_PROPERTY(QStringList, mounts);
    Q_SERIALIZE_PROPERTY(NamespaceList, namespaces);
    Q_SERIALIZE_PTR_PROPERTY(Root, root);
    Q_SERIALIZE_PTR_PROPERTY(TestPermission, permissions);
};
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(TestApp)
