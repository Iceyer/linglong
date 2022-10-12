/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     KeZhihuan <kezhihuan@uniontech.com>
 *
 * Maintainer: KeZhihuan <kezhihuan@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "app_status.h"
#include "module/repo/ostree_repo.h"
#include "module/repo/repo_client.h"
#include "module/util/job/job_controller.h"
#include "module/util/http/httpclient.h"
#include "module/dbus_ipc/package_manager_param.h"
#include "module/util/sysinfo.h"
#include "dbus_gen_system_helper_interface.h"
#include "package_database.h"

namespace linglong {
namespace package_manager {

class PackageManager;
class PackageManagerPrivate : public QObject
{
    Q_OBJECT
public:
    explicit PackageManagerPrivate(PackageManager *parent);
    ~PackageManagerPrivate() override = default;

public:
    std::tuple<util::Error, QVariantMapList> query(const package::Ref &ref, bool noCache);
    util::Error install(uid_t uid, const package::Ref &ref, const QVariantMap &options, util::Job *job);
    util::Error update(uid_t uid, const package::Ref &ref, const QVariantMap &options, util::Job *job);
    util::Error uninstall(uid_t uid, const package::Ref &ref, const QVariantMap &options);

    bool isUserAppInstalled(const QString &userName, const package::Ref &ref);
    bool isUserRuntimeInstalled(const QString &userName, const package::Ref &ref);
    bool isUserBaseInstalled(const QString &userName, const package::Ref &ref);

    std::tuple<util::Error, std::unique_ptr<package::MetaInfo>> getLatestPackageMetaInfo(const package::Ref &ref);

    package::Ref getRuntimeBaseRef(const package::Ref &ref);

    QList<package::Ref> findRefsToUninstall(const package::Ref &ref, bool deleteAllVersion);

    util::Error exportFiles(const package::Ref &ref);
    util::Error prunePackageFiles(const package::Ref &ref);

    /*
     * 从json字符串中提取软件包对应的JsonArray数据
     *
     * @param jsonString: 软件包对应的json字符串
     * @param jsonValue: 转换结果
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool getAppJsonArray(const QString &jsonString, QJsonValue &jsonValue, QString &err);

    /*
     * 将json字符串转化为软件包数据
     *
     * @param jsonString: 软件包对应的json字符串
     * @param appList: 转换结果
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool loadAppInfo(const QString &jsonString, linglong::package::MetaInfoList &appList, QString &err);

    /*
     * 从服务器查询指定包名/版本/架构的软件包数据
     *
     * @param pkgName: 软件包包名
     * @param pkgVer: 软件包版本号
     * @param pkgArch: 软件包对应的架构
     * @param appData: 查询结果
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool getAppInfoFromServer(const QString &pkgName, const QString &pkgVer, const QString &pkgArch, QString &appData,
                              QString &err);

    /*
     * 安装应用时更新包括desktop文件在内的配置文件
     *
     * @param appId: 应用的appId
     * @param version: 应用的版本号
     * @param arch: 应用对应的架构
     */
    void addAppConfig(const QString &appId, const QString &version, const QString &arch);

    /*
     * 卸载应用时更新包括desktop文件在内的配置文件
     *
     * @param appId: 应用的appId
     * @param version: 应用的版本号
     * @param arch: 应用对应的架构
     */
    void delAppConfig(const QString &appId, const QString &version, const QString &arch);

private:
    const QString entriesSharePath;
    const QString kAppInstallPath;
    const QString kLocalRepoPath;
    const QString kRemoteRepoName;

    PackageDatabase pdb;

    repo::OSTreeRepo ostreeRepo;
    repo::RepoClient repoClient;

    OrgDeepinLinglongSystemHelperInterface systemHelperInterface;

public:
    PackageManager *const q_ptr;
    Q_DECLARE_PUBLIC(PackageManager);
};
} // namespace package_manager
} // namespace linglong
