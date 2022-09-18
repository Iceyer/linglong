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

#include <QObject>
#include <QScopedPointer>

#include "module/util/serialize/json.h"
#include "module/package/ref.h"
#include "module/package/package.h"

namespace linglong {
namespace builder {

const char *const kDependTypeRuntime = "runtime";
const char *const kBuildScriptPath = "/entry.sh";

const char *const kBuildCacheDirectoryName = "linglong-builder";

const char *const kPackageKindApp = "app";
const char *const kPackageKindLib = "lib";
const char *const kPackageKindRuntime = "runtime";

class Variables : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Variables)
public:
    Q_SERIALIZE_PROPERTY(QString, CC);
    Q_SERIALIZE_PROPERTY(QString, CXX);
    Q_SERIALIZE_PROPERTY(QString, NM);
    Q_SERIALIZE_PROPERTY(QString, AR);
    Q_SERIALIZE_PROPERTY(QString, CFLAGS);
    Q_SERIALIZE_PROPERTY(QString, CXXFLAGS);
    Q_SERIALIZE_PROPERTY(QString, LDFLAGS);
    Q_SERIALIZE_PROPERTY(QString, id);
    Q_SERIALIZE_PROPERTY(QString, triplet);
    Q_SERIALIZE_PROPERTY(QString, build_dir);
    Q_SERIALIZE_PROPERTY(QString, dest_dir);
    Q_SERIALIZE_PROPERTY(QString, conf_args);
    Q_SERIALIZE_PROPERTY(QString, extra_args);
    Q_SERIALIZE_PROPERTY(QString, jobs);
};

class Package : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Package)
public:
    Q_SERIALIZE_PROPERTY(QString, id);
    Q_SERIALIZE_PROPERTY(QString, kind);
    Q_SERIALIZE_PROPERTY(QString, name);
    Q_SERIALIZE_PROPERTY(QString, version);
    Q_SERIALIZE_PROPERTY(QString, description);
};

class BuilderRuntime : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(BuilderRuntime)
public:
    Q_SERIALIZE_PROPERTY(QString, id);
    Q_SERIALIZE_PROPERTY(QString, version);
    //    Q_SERIALIZE_PROPERTY(QString, locale);
};

class Source : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Source)
public:
    Q_SERIALIZE_PROPERTY(QString, kind);

    // diff with kind
    Q_SERIALIZE_PROPERTY(QString, url);
    //! the unique id of digest
    Q_SERIALIZE_PROPERTY(QString, digest);
    Q_SERIALIZE_PROPERTY(QString, version);

    // git
    Q_SERIALIZE_PROPERTY(QString, commit);

    Q_SERIALIZE_PROPERTY(QStringList, patch);
};

class BuildManual : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(BuildManual)
public:
    Q_SERIALIZE_PROPERTY(QString, configure);
    Q_SERIALIZE_PROPERTY(QString, build);
    Q_SERIALIZE_PROPERTY(QString, install);
};

class Build : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Build)
public:
    Q_SERIALIZE_PROPERTY(QString, kind);
    Q_SERIALIZE_PTR_PROPERTY(BuildManual, manual);
};

} // namespace builder
} // namespace linglong

namespace linglong {
namespace builder {

class BuildDepend : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(BuildDepend)
public:
    Q_SERIALIZE_PROPERTY(QString, id);
    Q_SERIALIZE_PROPERTY(QString, version);
    Q_SERIALIZE_PROPERTY(QString, type);

    Q_SERIALIZE_PTR_PROPERTY(Variables, variables);
    Q_SERIALIZE_PTR_PROPERTY(Source, source);
    Q_SERIALIZE_PTR_PROPERTY(Build, build);
};
} // namespace builder
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, BuildDepend)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, Variables)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, Package)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, BuilderRuntime)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, Source)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, Build)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, BuildManual)

namespace linglong {
namespace builder {

class ProjectPrivate;
class Project : public Serialize
{
    Q_OBJECT;

public:
    explicit Project(QObject *parent = nullptr);
    ~Project() override;

public:
    Q_SERIALIZE_PTR_PROPERTY(Variables, variables);
    Q_SERIALIZE_PTR_PROPERTY(Package, package);
    Q_SERIALIZE_PTR_PROPERTY(BuilderRuntime, runtime);
    Q_SERIALIZE_PTR_PROPERTY(BuilderRuntime, base);
    // WARNING: the default meta id is expanded form here, so keep it as full of namespace.
    Q_SERIALIZE_PROPERTY(linglong::builder::BuildDependList, depends);
    Q_SERIALIZE_PTR_PROPERTY(Source, source);
    Q_SERIALIZE_PTR_PROPERTY(Build, build);

public:
    package::Ref ref() const;
    package::Ref fullRef(const QString &channel, const QString &module) const;
    package::Ref refWithModule(const QString &module) const;
    package::Ref runtimeRef() const;
    package::Ref baseRef() const;

    QString configFilePath() const;
    void setConfigFilePath(const QString &configFilePath);

    QString buildScriptPath() const;

public:
    class Config
    {
    public:
        explicit Config(const QString &root, const Project *project)
            : m_projectRoot(root)
            , m_project(project)
        {
        }

        QString absoluteFilePath(const QStringList &filenames) const;

        // ${PROJECT_ROOT}/.linglong-target
        QString rootPath() const;
        QString cacheAbsoluteFilePath(const QStringList &filenames) const;
        QString cacheRuntimePath(const QString &subPath) const;
        QString cacheInstallPath(const QString &subPath) const;
        QString cacheInstallPath(const QString &moduleDir, const QString &subPath) const;

        QString targetArch() const;

        // in container, /runtime or /opt/apps/${appid}
        QString targetInstallPath(const QString &sub) const;

    private:
        const QString m_projectRoot;
        const Project *m_project;
    };

    const Config &config() const;

    void onPostSerialize() override;

    void generateBuildScript();

private:
    QScopedPointer<ProjectPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), Project)
};

package::Ref fuzzyRef(const Serialize *obj);

} // namespace builder
} // namespace linglong

namespace linglong {
namespace builder {
class Template : public Serialize
{
    Q_OBJECT;

public:
    Q_SERIALIZE_PTR_PROPERTY(Variables, variables);
    Q_SERIALIZE_PTR_PROPERTY(Build, build);
};
} // namespace builder
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::builder, Project)
