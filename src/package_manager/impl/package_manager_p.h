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
#include "module/util/httpclient.h"
#include "module/util/package_manager_param.h"
#include "module/util/sysinfo.h"
#include "dbus_gen_system_helper_interface.h"

namespace linglong {
namespace package_manager {

class Job;
class PackageManager;
class PackageManagerPrivate : public QObject
{
    Q_OBJECT
public:
    explicit PackageManagerPrivate(PackageManager *parent);
    ~PackageManagerPrivate() override = default;

    bool isUserAppInstalled(const QString &userName, const package::Ref &ref);
    bool isUserRuntimeInstalled(const QString &userName, const package::Ref &ref);
    bool isUserBaseInstalled(const QString &userName, const package::Ref &ref);

    std::tuple<util::Error, std::unique_ptr<package::AppMetaInfo>> getLatestPackageMetaInfo(const package::Ref &ref);

    package::Ref getRuntimeBaseRef(const package::Ref &ref);

    util::Error exportFiles(const package::Ref &ref);

private:
    util::Error install(const package::Ref &ref, Job *job);

    Reply Install(const InstallParamOption &installParamOption);
    Reply Uninstall(const UninstallParamOption &paramOption);
    QueryReply Query(const QueryParamOption &paramOption);
    Reply Update(const ParamOption &paramOption);

    /**
     * @brief 查询软件包下载安装状态
     *
     * @param paramOption 查询参数
     * @param type 查询类型 0:查询应用安装进度 1:查询应用更新进度
     *
     * @return Reply dbus方法调用应答 \n
     *          code:状态码 \n
     *          message:信息
     */
    Reply GetDownloadStatus(const ParamOption &paramOption, int type);

    /*
     * 从给定的软件包列表中查找最新版本的软件包
     *
     * @param appId: 待匹配应用的appId
     * @param appList: 待搜索的软件包列表信息
     *
     * @return AppMetaInfo: 最新版本的软件包
     *
     */
    linglong::package::AppMetaInfo *getLatestApp(const QString &appId,
                                                 const linglong::package::AppMetaInfoList &appList);

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
    bool loadAppInfo(const QString &jsonString, linglong::package::AppMetaInfoList &appList, QString &err);

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
    bool getAppInfofromServer(const QString &pkgName, const QString &pkgVer, const QString &pkgArch, QString &appData,
                              QString &err);
    /*
     * 将在线包数据部分签出到指定目录
     *
     * @param pkgName: 软件包包名
     * @param pkgVer: 软件包版本号
     * @param pkgArch: 软件包对应的架构
     * @param dstPath: 在线包数据部分存储路径
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool downloadAppData(const QString &pkgName, const QString &pkgVer, const QString &pkgArch, const QString &channel,
                         const QString &module, const QString &dstPath, QString &err);

    /*
     * 安装应用runtime
     *
     * @param runtimeId: runtime对应的appId
     * @param runtimeVer: runtime版本号
     * @param runtimeArch: runtime对应的架构
     * @param channel: 软件包对应的渠道
     * @param module: 软件包类型
     * @param err: 错误信息
     *
     * @return bool: true:成功 false:失败
     */
    bool installRuntime(const QString &runtimeId, const QString &runtimeVer, const QString &runtimeArch,
                        const QString &channel, const QString &module, QString &err);

    /*
     * 检查应用runtime安装状态
     *
     * @param runtime: 应用runtime字符串
     * @param channel: 软件包对应的渠道
     * @param module: 软件包类型
     * @param err: 错误信息
     *
     * @return bool: true:安装成功或已安装返回true false:安装失败
     */
    bool checkAppRuntime(const QString &runtime, const QString &channel, const QString &module, QString &err);

    /*
     * 针对非deepin发行版检查应用base安装状态
     *
     * @param runtime: runtime ref
     * @param channel: 软件包对应的渠道
     * @param module: 软件包类型
     * @param err: 错误信息
     *
     * @return bool: true:安装成功或已安装返回true false:安装失败
     */
    bool checkAppBase(const QString &runtime, const QString &channel, const QString &module, QString &err);

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

    /*
     * 通过用户uid获取对应的用户名
     *
     * @param uid: 用户uid
     *
     * @return QString: uid对应的用户名
     */
    QString getUserName(uid_t uid);

private:
    const QString sysLinglongInstalltions;
    const QString kAppInstallPath;
    const QString kLocalRepoPath;
    const QString kRemoteRepoName;

    // 记录子线程安装及更新状态 供查询进度信息使用
    QMap<QString, Reply> appState;
    OrgDeepinLinglongSystemHelperInterface systemHelperInterface;

    bool noDBusMode = false;

    repo::OSTreeRepo ostreeRepo;
    repo::RepoClient repoClient;

public:
    PackageManager *const q_ptr;
    Q_DECLARE_PUBLIC(PackageManager);
};
} // namespace package_manager
} // namespace linglong
