/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_REPO_OSTREE_H_
#define LINGLONG_SRC_MODULE_REPO_OSTREE_H_

#include <QObject>
#include <QScopedPointer>
#include <QPointer>
#include <QMap>
#include <QLoggingCategory>

#include "module/repo/repo.h"
#include "module/package/package.h"

namespace linglong {
namespace repo {

class OSTreeRepoPrivate;
class OSTreeRepo
    : public QObject
    , public Repo
{
    Q_OBJECT
public:
    enum Mode {
        Bare,
        BareUser,
        BareUserOnly,
        Archive,
    };
    Q_ENUM(Mode);

    explicit OSTreeRepo(const QString &path);
    explicit OSTreeRepo(const QString &localRepoPath, const QString &remoteEndpoint, const QString &remoteRepoName);
    ~OSTreeRepo() override;

    util::Error init(const QString &repoMode);

    util::Error remoteAdd(const QString &repoName, const QString &repoUrl);

    std::tuple<util::Error, QStringList> remoteList();

    util::Error importDirectory(const package::Ref &ref, const QString &path) override;

    util::Error import(const package::Bundle &bundle) override;

    util::Error exportBundle(package::Bundle &bundle) override;

    std::tuple<util::Error, QList<package::Ref>> list(const QString &filter) override;

    std::tuple<util::Error, QList<package::Ref>> query(const QString &filter) override;

    util::Error push(const package::Ref &ref, bool force) override;

    util::Error push(const package::Bundle &bundle, bool force) override;

    util::Error pull(const package::Ref &ref, bool force) override;

    util::Error pull(const package::Ref &ref, QObject *controller = nullptr);

    util::Error pullAll(const package::Ref &ref, bool force);

    util::Error checkout(const package::Ref &ref, const QString &subPath, const QString &target,
                         const QStringList &args = {});

    util::Error remove(const package::Ref &ref);

    util::Error checkoutAll(const package::Ref &ref, const QString &subPath, const QString &target);

    QString rootOfLayer(const package::Ref &ref) override;

    bool isRefExists(const package::Ref &ref);

    package::Ref remoteLatestRef(const package::Ref &ref);

    package::Ref latestOfRef(const QString &appId, const QString &appVersion) override;

Q_SIGNALS:
    void pullProgressChanged(const QVariantMap &extraData);

private:
    QScopedPointer<OSTreeRepoPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), OSTreeRepo)
};

} // namespace repo
} // namespace linglong

namespace linglong {
namespace repo {

class InfoResponse : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(InfoResponse)

    Q_SERIALIZE_PROPERTY(QStringMap, revs);
};

class RevPair : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(RevPair)

    Q_SERIALIZE_PROPERTY(QString, server);
    Q_SERIALIZE_PROPERTY(QString, client);
};

} // namespace repo
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::repo, InfoResponse)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::repo, RevPair)

namespace linglong {
namespace repo {

class UploadTaskRequest : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(UploadTaskRequest)

    Q_SERIALIZE_PROPERTY(int, code);
    Q_SERIALIZE_PROPERTY(QStringList, objects);
    Q_SERIALIZE_PROPERTY(linglong::repo::RevPairStrMap, refs);
};

class UploadTaskResponse : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(UploadTaskResponse)

    Q_SERIALIZE_PROPERTY(QString, id);
};

} // namespace repo
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::repo, UploadTaskRequest)
Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::repo, UploadTaskResponse)

Q_DECLARE_LOGGING_CATEGORY(repoProgress)

#endif // LINGLONG_SRC_MODULE_REPO_OSTREE_H_
