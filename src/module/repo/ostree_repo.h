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
    linglong::util::Error init(const QString &repoMode);

    linglong::util::Error remoteAdd(const QString &repoName, const QString &repoUrl);

    std::tuple<linglong::util::Error, QStringList> remoteList();

    linglong::util::Error importDirectory(const package::Ref &ref, const QString &path) override;

    linglong::util::Error import(const package::Bundle &bundle) override;

    linglong::util::Error exportBundle(package::Bundle &bundle) override;

    std::tuple<linglong::util::Error, QList<package::Ref>> list(const QString &filter) override;

    std::tuple<linglong::util::Error, QList<package::Ref>> query(const QString &filter) override;

    linglong::util::Error push(const package::Ref &ref, bool force) override;

    linglong::util::Error push(const package::Bundle &bundle, bool force) override;

    linglong::util::Error pull(const package::Ref &ref, bool force) override;

    linglong::util::Error pullAll(const package::Ref &ref, bool force);

    linglong::util::Error checkout(const package::Ref &ref, const QString &subPath, const QString &target);

    linglong::util::Error removeRef(const package::Ref &ref);

    linglong::util::Error checkoutAll(const package::Ref &ref, const QString &subPath, const QString &target);

    QString rootOfLayer(const package::Ref &ref) override;

    bool isRefExists(const package::Ref &ref);

    package::Ref localLatestRef(const package::Ref &ref);

    package::Ref remoteLatestRef(const package::Ref &ref);

    package::Ref latestOfRef(const QString &appId, const QString &appVersion) override;

Q_SIGNALS:
    void taskProgressChange(const QString &id, quint32 progress, const QString &message);

private:
    QScopedPointer<OSTreeRepoPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), OSTreeRepo)
};

} // namespace repo
} // namespace linglong

namespace linglong {
namespace repo {

class InfoResponse : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(InfoResponse)

    Q_JSON_PROPERTY(ParamStringMap, revs);
};

class RevPair : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(RevPair)

    Q_JSON_PROPERTY(QString, server);
    Q_JSON_PROPERTY(QString, client);
};

} // namespace repo
} // namespace linglong

Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, InfoResponse)
Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, RevPair)

namespace linglong {
namespace repo {

class UploadTaskRequest : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(UploadTaskRequest)

    Q_JSON_PROPERTY(int, code);
    Q_JSON_PROPERTY(QStringList, objects);
    Q_JSON_PROPERTY(linglong::repo::RevPairStrMap, refs);
};

class UploadTaskResponse : public JsonSerialize
{
    Q_OBJECT;
    Q_JSON_CONSTRUCTOR(UploadTaskResponse)

    Q_JSON_PROPERTY(QString, id);
};

} // namespace repo
} // namespace linglong

Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, UploadTaskRequest)
Q_JSON_DECLARE_PTR_METATYPE_NM(linglong::repo, UploadTaskResponse)

#endif // LINGLONG_SRC_MODULE_REPO_OSTREE_H_
