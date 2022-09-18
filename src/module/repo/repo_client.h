/*
* Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
*
* Author:     Iceyer <me@iceyer.net>
*
* Maintainer: Iceyer <me@iceyer.net>
*
* SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_REPO_REPO_CLIENT_H_
#define LINGLONG_SRC_MODULE_REPO_REPO_CLIENT_H_

#include <tuple>

#include <QNetworkReply>
#include <QNetworkRequest>

#include "module/util/result.h"
#include "module/package/ref.h"
#include "module/package/package.h"
#include "module/package/info.h"

namespace linglong {
namespace repo {

class Response : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Response)
    Q_SERIALIZE_PROPERTY(int, code);
    Q_SERIALIZE_PROPERTY(QString, msg);
    Q_SERIALIZE_PROPERTY(linglong::package::AppMetaInfoList, data);
};

class RepoClient
{
public:
//    RepoClient(const QString& repoPath);

    std::tuple<util::Error, package::AppMetaInfoList> QueryApps(const package::Ref& ref);

private:

};

} // namespace repo
} // namespace linglong

Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(linglong::repo, Response)

#endif // LINGLONG_SRC_MODULE_REPO_REPO_CLIENT_H_
