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

#include "oci.h"
#include "container.h"
#include "module/package/ref.h"

namespace linglong {
namespace repo {
class Repo;
}
} // namespace linglong

namespace linglong {
namespace runtime {

class Layer : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Layer)
    Q_SERIALIZE_PROPERTY(QString, ref);
};
Q_SERIALIZE_DECLARE_TYPE(Layer);


/*!
 * Permission: base for run, you can use full run or let it empty
 */
class Permissions : public Serialize
{
    Q_OBJECT
    Q_SERIALIZE_CONSTRUCTOR(Permissions)
    Q_SERIALIZE_PROPERTY(MountList, mounts);
};
Q_SERIALIZE_DECLARE_TYPE(Permissions);

class AppPrivate;
class App : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_PROPERTY(QString, version);
    Q_SERIALIZE_PTR_PROPERTY(linglong::runtime::Layer, package);
    Q_SERIALIZE_PTR_PROPERTY(linglong::runtime::Layer, runtime);

    // TODO: should config base mount point
    Q_SERIALIZE_PTR_PROPERTY(linglong::runtime::Permissions, permissions);

public:
    explicit App(QObject *parent = nullptr);
    ~App() override;

    static App *load(linglong::repo::Repo *repo, const linglong::package::Ref &ref, const QString &desktopExec,
                     bool useFlatpakRuntime);

    Container *container() const;

    int start();

    void exec(QString cmd, QString env, QString cwd);

    void saveUserEnvList(const QStringList &userEnvList);

    void setAppParamMap(const QStringMap &paramMap);

private:
    QScopedPointer<AppPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), App)
};
Q_SERIALIZE_DECLARE_TYPE(App);

} // namespace runtime
} // namespace linglong

Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::runtime, Layer)
Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::runtime, Permissions)
Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::runtime, App)
