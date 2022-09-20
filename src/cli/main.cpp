/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QMap>

#include "dbus_gen_app_manager_interface.h"
#include "dbus_gen_job_interface.h"
#include "dbus_gen_package_manager_interface.h"
#include "module/package/package.h"
#include "module/util/package_manager_param.h"
#include "module/dbus_ipc/dbus_app_manager_common.h"
#include "module/dbus_ipc/dbus_package_manager_common.h"
#include "module/runtime/runtime.h"
#include "module/util/xdg.h"
#include "module/util/env.h"
#include "module/util/log_handler.h"
#include "module/util/sysinfo.h"
#include "package_manager/impl/package_manager.h"
#include "package_manager/impl/app_status.h"
#include "service/impl/register_meta_type.h"
#include "service/impl/app_manager.h"

#include "cmd/command_helper.h"
#include "cli.h"

/**
 * @brief 注册 QT 对象类型
 *
 */
static void qSerializeRegisterAll()
{
    linglong::package::registerAllMetaType();
    linglong::runtime::registerAllMetaType();
    linglong::service::registerAllMetaType();
}

/**
 * @brief 输出 flatpak 命令的查询结果
 *
 * @param appMetaInfoList 软件包元信息列表
 *
 */
void printFlatpakAppInfo(linglong::package::AppMetaInfoList appMetaInfoList)
{
    if (appMetaInfoList.size() > 0) {
        if ("flatpaklist" == appMetaInfoList.at(0)->appId) {
            qInfo("%-48s%-16s%-16s%-12s%-12s%-12s%-12s", "Description", "Application", "Version", "Branch", "Arch",
                  "Origin", "Installation");
        } else {
            qInfo("%-72s%-16s%-16s%-12s%-12s", "Description", "Application", "Version", "Branch", "Remotes");
        }
        QString ret = appMetaInfoList.at(0)->description;
        QStringList strList = ret.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
        for (int i = 0; i < strList.size(); ++i) {
            qInfo().noquote() << strList[i].simplified();
        }
    }
}

/**
 * @brief 统计字符串中中文字符的个数
 *
 * @param name 软件包名称
 * @return int 中文字符个数
 */
int getUnicodeNum(const QString &name)
{
    int num = 0;
    int count = name.count();
    for (int i = 0; i < count; i++) {
        QChar ch = name.at(i);
        ushort decode = ch.unicode();
        if (decode >= 0x4E00 && decode <= 0x9FA5) {
            num++;
        }
    }
    return num;
}

/**
 * @brief 处理安装中断请求
 *
 * @param sig 中断信号
 */
void doIntOperate(int sig)
{
    // 显示光标
    std::cout << "\033[?25h" << std::endl;
    // Fix to 调用 jobManager 中止下载安装操作
    exit(0);
}

/**
 * @brief 输出软件包的查询结果
 *
 * @param appMetaInfoList 软件包元信息列表
 *
 */
void printAppInfo(linglong::package::AppMetaInfoList appMetaInfoList)
{
    if (appMetaInfoList.size() > 0) {
        qInfo("\033[1m\033[38;5;214m%-32s%-32s%-16s%-12s%-16s%-12s%-s\033[0m", qUtf8Printable("appId"),
              qUtf8Printable("name"), qUtf8Printable("version"), qUtf8Printable("arch"), qUtf8Printable("channel"),
              qUtf8Printable("module"), qUtf8Printable("description"));
        for (auto const &it : appMetaInfoList) {
            QString simpleDescription = it->description.trimmed();
            if (simpleDescription.length() > 56) {
                simpleDescription = it->description.trimmed().left(53) + "...";
            }
            QString appId = it->appId.trimmed();
            QString name = it->name.trimmed();
            if (name.length() > 32) {
                name = it->name.trimmed().left(29) + "...";
            }
            if (appId.length() > 32) {
                name.push_front(" ");
            }
            int count = getUnicodeNum(name);
            int length = simpleDescription.length() < 56 ? simpleDescription.length() : 56;
            qInfo().noquote() << QString("%1%2%3%4%5%6%7")
                                     .arg(appId, -32, QLatin1Char(' '))
                                     .arg(name, count - 32, QLatin1Char(' '))
                                     .arg(it->version.trimmed(), -16, QLatin1Char(' '))
                                     .arg(it->arch.trimmed(), -12, QLatin1Char(' '))
                                     .arg(it->channel.trimmed(), -16, QLatin1Char(' '))
                                     .arg(it->module.trimmed(), -12, QLatin1Char(' '))
                                     .arg(simpleDescription, -length, QLatin1Char(' '));
        }
    } else {
        qInfo().noquote() << "app not found in repo";
    }
}

void startDaemon(QString program, QStringList args = {})
{
    QProcess process;
    process.setProgram(program);
    process.setStandardOutputFile("/dev/null");
    process.setStandardErrorFile("/dev/null");
    process.setArguments(args);
    process.startDetached();
}

/**
 * @brief 检测 ll-service dbus 服务是否已经启动，未启动则启动
 *
 * @param packageManager ll-service dbus 服务
 *
 */
void checkAndStartService(OrgDeepinLinglongAppManagerInterface &packageManager)
{
    const auto kStatusActive = "active";
    QDBusReply<QString> status = packageManager.Status();
    // FIXME: should use more precision to check status
    if (kStatusActive != status.value()) {
        startDaemon("ll-service", {});

        for (int i = 0; i < 10; ++i) {
            status = packageManager.Status();
            if (kStatusActive == status.value()) {
                return;
            }
            QThread::sleep(1);
        }

        qCritical() << "start ll-service failed";
    }
}

void addRefArguments(QCommandLineParser &parser, QList<QCommandLineOption> opts)
{
    parser.addPositionalArgument("ref",
                                 R"MLS00(The full ref is:
[repo]/[channel]:{id}/[version]/[arch]/[module]
like:
"default/main:org.deepin.demo/1.0.0.1/arch/module"
only package id required，use this for short:
"main:org.deepin.demo/1.0.0.1" from main channel
"org.deepin.demo/1.0.0.1" from default channel
)MLS00",
                                 "org.deepin.demo");

    for (auto opt : opts) {
        parser.addOption(opt);
    }
}

using namespace linglong;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("deepin");

    // 安装消息处理函数
    LOG_HANDLER->installMessageHandler();

    // 注册 QT 对象类型
    qSerializeRegisterAll();

    QCommandLineParser parser;
    parser.addHelpOption();
    QStringList subCommandList = {"run",       "ps",     "exec",  "kill", "install",
                                  "uninstall", "update", "query", "list", "repo"};

    parser.addPositionalArgument("subcommand", subCommandList.join("\n"), "subcommand [sub-option]");

    // TODO: change to no-dbus-daemon
    auto optNoDbus = QCommandLineOption("nodbus", "execute cmd directly, not via dbus(only for root user)", "");
    optNoDbus.setFlags(QCommandLineOption::HiddenFromHelp);

    auto optRepoPoint = QCommandLineOption("repo-point", "app repo type to use", "repo-point", "");
    // TODO: may not support flatpak from cli, hidden it now.
    optRepoPoint.setFlags(QCommandLineOption::HiddenFromHelp);

    parser.addOptions({optNoDbus, optRepoPoint});

    parser.parse(QCoreApplication::arguments());

    QStringList args = parser.positionalArguments();
    QString command = args.isEmpty() ? QString() : args.first();

    auto appManagerDBusConnection = QDBusConnection::sessionBus();
    auto packageManagerDBusConnection = QDBusConnection::systemBus();
    auto systemHelperDBusConnection = QDBusConnection::systemBus();

    auto systemHelperAddress = QString("unix:path=/run/linglong_system_helper_socket");
    auto packageManagerAddress = QString("unix:path=/run/linglong_package_manager_socket");
    auto appManagerAddress = QString("unix:path=/run/linglong_app_manager_socket");

    if (parser.isSet(optNoDbus)) {
        // NOTE: isConnected will NOT RETRY
        // NOTE: name cannot be duplicate
        systemHelperDBusConnection = QDBusConnection::connectToPeer(systemHelperAddress, "ll-system-helper-1");
        if (!systemHelperDBusConnection.isConnected()) {
            startDaemon("ll-system-helper", {"--bus=" + systemHelperAddress});
            QThread::sleep(2);
            systemHelperDBusConnection = QDBusConnection::connectToPeer(systemHelperAddress, "ll-system-helper");
            if (!systemHelperDBusConnection.isConnected()) {
                qCritical() << "failed to start ll-system-helper";
                exit(-1);
            }
        }
        setenv("LINGLONG_SYSTEM_HELPER_ADDRESS", systemHelperAddress.toStdString().c_str(), true);

        packageManagerDBusConnection = QDBusConnection::connectToPeer(packageManagerAddress, "ll-package-manager-1");
        if (!packageManagerDBusConnection.isConnected()) {
            startDaemon("ll-package-manager", {"--bus=" + packageManagerAddress});
            QThread::sleep(2);
            packageManagerDBusConnection = QDBusConnection::connectToPeer(packageManagerAddress, "ll-package-manager");
            if (!packageManagerDBusConnection.isConnected()) {
                qCritical() << "failed to start ll-package-manager";
                exit(-1);
            }
        }
        setenv("LINGLONG_PACKAGE_MANAGER_ADDRESS", packageManagerAddress.toStdString().c_str(), true);

        appManagerDBusConnection = QDBusConnection::connectToPeer(appManagerAddress, "ll-service-1");
        if (!appManagerDBusConnection.isConnected()) {
            startDaemon("ll-service", {"--bus=" + appManagerAddress});
            QThread::sleep(2);
            appManagerDBusConnection = QDBusConnection::connectToPeer(appManagerAddress, "ll-service");
            if (!appManagerDBusConnection.isConnected()) {
                qCritical() << "failed to start ll-service";
                exit(-1);
            }
        }
        setenv("LINGLONG_APP_MANAGER_ADDRESS", appManagerAddress.toStdString().c_str(), true);
    }

    OrgDeepinLinglongAppManagerInterface appManager(AppManagerDBusServiceName, AppManagerDBusPath,
                                                    appManagerDBusConnection);

    OrgDeepinLinglongPackageManagerInterface packageManager(DBusPackageManagerServiceName, DBusPackageManagerPath,
                                                            packageManagerDBusConnection);
    if (!parser.isSet(optNoDbus)) {
        // NOTE: should be fine to call this using peer to peer dbus
        checkAndStartService(appManager);
    }

    // some common option
    auto optChannel = QCommandLineOption("channel", "The channel of package", kDefaultChannel, kDefaultChannel);
    auto optModule = QCommandLineOption("module", "The module of package", kDefaultModule, kDefaultModule);

    QMap<QString, std::function<int(QCommandLineParser & parser)>> subcommandMap = {
        {"run", // 启动玲珑应用
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("run", "run application", "run");
             parser.addPositionalArgument("appId", "application id", "com.deepin.demo");

             auto optExec = QCommandLineOption("exec", "run exec", "/bin/bash");
             parser.addOption(optExec);

             auto optNoProxy = QCommandLineOption("no-proxy", "whether to use dbus proxy in box", "");

             auto optNameFilter = QCommandLineOption("filter-name", "dbus name filter to use",
                                                     "--filter-name=com.deepin.linglong.AppManager", "");
             auto optPathFilter = QCommandLineOption("filter-path", "dbus path filter to use",
                                                     "--filter-path=/com/deepin/linglong/PackageManager", "");
             auto optInterfaceFilter = QCommandLineOption("filter-interface", "dbus interface filter to use",
                                                          "--filter-interface=com.deepin.linglong.PackageManager", "");

             // 增加channel/module
             auto optChannel = QCommandLineOption("channel", "the channnel of app", "--channel=linglong", "linglong");
             parser.addOption(optChannel);
             auto optModule = QCommandLineOption("module", "the module of app", "--module=runtime", "runtime");
             parser.addOption(optModule);

             parser.addOption(optNoProxy);
             parser.addOption(optNameFilter);
             parser.addOption(optPathFilter);
             parser.addOption(optInterfaceFilter);
             parser.process(app);
             auto repoType = parser.value(optRepoPoint);
             if ((!repoType.isEmpty() && "flatpak" != repoType)) {
                 parser.showHelp();
                 return -1;
             }
             args = parser.positionalArguments();
             auto appId = args.value(1);
             if (appId.isEmpty()) {
                 parser.showHelp();
                 return -1;
             }

             auto exec = parser.value(optExec);
             // 转化特殊字符
             args = linglong::util::convertSpecialCharacters(args);

             // 移除 run appid 两个参数 获取 exec 执行参数
             // eg: ll-cli run deepin-music --exec deepin-music /usr/share/music/test.mp3
             // exec = "deepin-music /usr/share/music/test.mp3"
             QString desktopArgs;
             desktopArgs.clear();
             if (args.length() > 2 && !exec.isEmpty()) {
                 args.removeAt(0);
                 args.removeAt(0);
                 desktopArgs = args.join(" ");
                 exec = QStringList {exec, desktopArgs}.join(" ");
             }

             linglong::service::RunParamOption paramOption;
             // appId format: org.deepin.calculator/1.2.6 in multi-version
             QMap<QString, QString> paramMap;
             QStringList appInfoList = appId.split("/");
             paramOption.appId = appId;
             if (appInfoList.size() > 1) {
                 paramOption.appId = appInfoList.at(0);
                 paramOption.version = appInfoList.at(1);
             }
             if (!repoType.isEmpty()) {
                 paramOption.repoPoint = repoType;
             }

             if (!exec.isEmpty()) {
                 paramOption.exec = exec;
             }

             paramOption.channel = parser.value(optChannel);
             paramOption.appModule = parser.value(optModule);

             // 获取用户环境变量
             QStringList envList = COMMAND_HELPER->getUserEnv(linglong::util::envList);
             if (!envList.isEmpty()) {
                 paramOption.appEnv = envList.join(",");
             }

             // 判断是否设置了 no-proxy 参数
             paramOption.noDbusProxy = parser.isSet(optNoProxy);
             if (!parser.isSet(optNoProxy)) {
                 // FIX to do only deal with session bus
                 paramOption.busType = "session";
                 paramOption.filterName = parser.value(optNameFilter);
                 paramOption.filterPath = parser.value(optPathFilter);
                 paramOption.filterInterface = parser.value(optInterfaceFilter);
             }

             // ll-cli 进沙箱环境
             linglong::service::Reply reply;
             if ("/bin/bash" == parser.value(optExec) || "bash" == parser.value(optExec)) {
                 reply = service::AppManager::instance()->Start(paramOption);
                 service::AppManager::instance()->pool().waitForDone(-1);
                 if (0 != reply.code) {
                     qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                     return -1;
                 }
                 return 0;
             }

             QDBusPendingReply<linglong::service::Reply> dbusReply = appManager.Start(paramOption);
             dbusReply.waitForFinished();
             reply = dbusReply.value();
             if (reply.code != 0) {
                 qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                 return -1;
             }
             return 0;
         }},
        {"exec", // exec new command in a existed container.
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("exec", "exec command in container", "exec");
             parser.addPositionalArgument("containerId", "container id", "aebbe2f455cf443f89d5c92f36d154dd");
             parser.addPositionalArgument("cmd", "command", "\"bash\"");
             auto envArg = QCommandLineOption({"e", "env"}, "extra environment variables splited by comma", "env", "");
             auto pwdArg =
                 QCommandLineOption({"d", "path"}, "location to exec the new command", "pwd", qgetenv("HOME"));
             parser.addOption(envArg);
             parser.addOption(pwdArg);
             parser.process(app);

             auto containerId = parser.positionalArguments().value(1);
             if (containerId.isEmpty()) {
                 parser.showHelp();
                 return -1;
             }

             auto cmd = parser.positionalArguments().value(2);
             if (cmd.isEmpty()) {
                 parser.showHelp();
                 return -1;
             }

             linglong::service::ExecParamOption p;
             p.cmd = cmd;
             p.containerID = containerId;

             auto envs = parser.value(envArg).split(",", QString::SkipEmptyParts);
             QStringList envList = envs;
             p.env = envList.join(",");
             //  for (auto env : envs) {
             //      auto pos = env.indexOf('=');
             //      auto key = QStringRef(&env, 0, pos), val = QStringRef(&env, pos + 1, env.length() - pos - 1);
             //      qputenv(key.toString().toStdString().c_str(), QByteArray(val.toString().toUtf8()));
             //  }

             // 获取用户环境变量
             //  QStringList envList = COMMAND_HELPER->getUserEnv(linglong::util::envList);
             //  if (!envList.isEmpty()) {
             //      p.env = envList.join(",");
             //  }

             auto dbusReply = appManager.Exec(p);
             auto reply = dbusReply.value();
             if (reply.code != STATUS_CODE(kSuccess)) {
                 qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                 return -1;
             }
             return 0;
         }},
        {"enter",
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("containerId", "container id", "aebbe2f455cf443f89d5c92f36d154dd");
             parser.addPositionalArgument("exec", "exec command in container", "/bin/bash");
             parser.process(app);

             auto containerId = parser.positionalArguments().value(1);
             if (containerId.isEmpty()) {
                 parser.showHelp();
                 return -1;
             }

             auto cmd = parser.positionalArguments().value(2);
             if (cmd.isEmpty()) {
                 parser.showHelp();
                 return -1;
             }

             auto pid = containerId.toInt();

             return COMMAND_HELPER->namespaceEnter(pid, QStringList {cmd});
         }},
        {"ps", // 查看玲珑沙箱进程
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("ps", "show running applications", "ps");

             auto optOutputFormat = QCommandLineOption("output-format", "json/console", "console");
             parser.addOption(optOutputFormat);

             parser.process(app);

             auto outputFormat = parser.value(optOutputFormat);
             auto replyString = appManager.ListContainer().value().result;

             ContainerList containerList;
             auto doc = QJsonDocument::fromJson(replyString.toUtf8(), nullptr);
             if (doc.isArray()) {
                 for (auto container : doc.array()) {
                     auto str = QString(QJsonDocument(container.toObject()).toJson());
                     QPointer<Container> con(linglong::util::loadJSONString<Container>(str));
                     containerList.push_back(con);
                 }
             }
             COMMAND_HELPER->showContainer(containerList, outputFormat);
             return 0;
         }},
        {"kill", // 关闭玲珑沙箱
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("kill", "kill container with id", "kill");
             parser.addPositionalArgument("container-id", "container id", "");

             parser.process(app);
             args = parser.positionalArguments();

             auto containerId = args.value(1).trimmed();
             if (containerId.isEmpty() || args.size() > 2) {
                 parser.showHelp();
                 return -1;
             }
             // TODO: show kill result
             QDBusPendingReply<linglong::service::Reply> dbusReply = appManager.Stop(containerId);
             dbusReply.waitForFinished();
             linglong::service::Reply reply = dbusReply.value();
             if (reply.code != STATUS_CODE(kErrorPkgKillSuccess)) {
                 qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                 return -1;
             }

             qInfo().noquote() << reply.message;
             return 0;
         }},
        {"install", // 下载玲珑包
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("install", "Install a package", "install");
             addRefArguments(parser, {optChannel, optModule});
             parser.process(app);

             args = parser.positionalArguments();
             auto repoType = parser.value(optRepoPoint);

             // 参数个数校验
             if (args.size() != 2 || (!repoType.isEmpty() && "flatpak" != repoType)) {
                 parser.showHelp(-1);
                 return -1;
             }

             // 收到中断信号后恢复操作
             signal(SIGINT, doIntOperate);

             auto argRef = args.at(1);
             package::Ref ref(argRef);

             if (parser.isSet(optChannel)) {
                 ref.channel = parser.value(optChannel);
             }
             if (parser.isSet(optModule)) {
                 ref.module = parser.value(optModule);
             }

             qInfo().noquote() << "install" << argRef << ", please wait a few minutes...";

             // if (parser.isSet(optNoDbus)) {
             // linglong::service::Reply reply;
             // // appId format: org.deepin.calculator/1.2.6 in multi-version
             // linglong::service::InstallParamOption installParamOption;
             // installParamOption.repoPoint = repoType;
             // // 增加 channel/module
             // installParamOption.channel = parser.value(optChannel);
             // installParamOption.appModule = parser.value(optModule);
             // QStringList appInfoList = args.at(1).split("/");
             // installParamOption.appId = appInfoList.at(0);
             // installParamOption.arch = linglong::util::hostArch();
             // if (appInfoList.size() == 2) {
             // installParamOption.version = appInfoList.at(1);
             // } else if (appInfoList.size() == 3) {
             // installParamOption.version = appInfoList.at(1);
             // installParamOption.arch = appInfoList.at(2);
             // }
             // SYSTEM_MANAGER_HELPER->setNoDBusMode(true);
             // reply = SYSTEM_MANAGER_HELPER->InstallNoDbus(installParamOption);
             // SYSTEM_MANAGER_HELPER->pool->waitForDone(-1);
             // qInfo().noquote() << "install " << installParamOption.appId << " done";
             // } else {
             qDebug() << "install spec ref" << ref.toSpecString();

             QDBusPendingReply<QString> dbusReply = packageManager.Install(ref.toSpecString(), {});
             dbusReply.waitForFinished();
             QString jobPath = dbusReply.value();

             QScopedPointer<linglong::cli::Cli> cli(new linglong::cli::Cli);
             QScopedPointer<QDBusInterface> jobInterface(new QDBusInterface(DBusPackageManagerServiceName, jobPath,
                                                                            DBusPackageManagerJobInterface,
                                                                            packageManagerDBusConnection, nullptr));
             QObject::connect(jobInterface.data(), SIGNAL(ProgressChanged(quint32, quint64, quint64, qint64, QString)),
                              cli.data(), SLOT(onJobProgressChanged(quint32, quint64, quint64, qint64, QString)));
             if (!parser.isSet(optNoDbus)) {
                 QObject::connect(jobInterface.data(), SIGNAL(Finish(quint32, QString)), cli.data(),
                                  SLOT(onFinish(quint32, QString)));
             } else {
                 while (jobInterface.data()->property("FinishCode").toUInt() == uint(util::StatusCode::kPkgUpdating)) {
                     QThread::sleep(1);
                 }
                 if (jobInterface.data()->property("FinishCode").toUInt())
                     exit(-1);
                 else {
                     return 0;
                 }
             }

             return app.exec();
             // }
         }},
        {"update", // 更新玲珑包
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("update", "Update an application", "update");
             addRefArguments(parser, {optChannel, optModule});

             parser.process(app);
             linglong::service::ParamOption paramOption;
             args = parser.positionalArguments();
             QStringList appInfoList = args.value(1).split("/");
             if (args.size() != 2) {
                 parser.showHelp(-1);
                 return -1;
             }
             paramOption.appId = appInfoList.at(0).trimmed();
             if (paramOption.appId.isEmpty()) {
                 parser.showHelp(-1);
                 return -1;
             }
             paramOption.arch = linglong::util::hostArch();
             if (appInfoList.size() > 1) {
                 paramOption.version = appInfoList.at(1);
             }
             // 增加 channel/module
             paramOption.channel = parser.value(optChannel);
             paramOption.appModule = parser.value(optModule);
             packageManager.setTimeout(1000 * 60 * 60 * 24);
             qInfo().noquote() << "update" << paramOption.appId << ", please wait a few minutes...";
             QDBusPendingReply<linglong::service::Reply> dbusReply = packageManager.Update(paramOption);
             dbusReply.waitForFinished();
             linglong::service::Reply reply;
             reply = dbusReply.value();
             if (reply.code == STATUS_CODE(kPkgUpdating)) {
                 signal(SIGINT, doIntOperate);
                 QThread::sleep(1);
                 dbusReply = packageManager.GetDownloadStatus(paramOption, 1);
                 dbusReply.waitForFinished();
                 reply = dbusReply.value();
                 bool disProgress = false;
                 // 隐藏光标
                 std::cout << "\033[?25l";
                 while (reply.code == STATUS_CODE(kPkgUpdating)) {
                     std::cout << "\r\33[K" << reply.message.toStdString();
                     std::cout.flush();
                     QThread::sleep(1);
                     dbusReply = packageManager.GetDownloadStatus(paramOption, 1);
                     dbusReply.waitForFinished();
                     reply = dbusReply.value();
                     disProgress = true;
                 }
                 // 显示光标
                 std::cout << "\033[?25h";
                 if (disProgress) {
                     std::cout << std::endl;
                 }
             }
             if (reply.code != STATUS_CODE(kErrorPkgUpdateSuccess)) {
                 qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                 return -1;
             }
             qInfo().noquote() << "message:" << reply.message;
             return 0;
         }},
        {"query", // 查询玲珑包
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("query", "Query app info", "query");
             addRefArguments(parser, {});

             auto optNoCache = QCommandLineOption("force", "Query from server directly, not from cache", "");
             parser.addOption(optNoCache);
             parser.process(app);

             args = parser.positionalArguments();
             auto repoType = parser.value(optRepoPoint);
             if (args.size() != 2 || (!repoType.isEmpty() && "flatpak" != repoType)) {
                 parser.showHelp(-1);
                 return -1;
             }

             linglong::service::QueryParamOption paramOption;
             paramOption.appId = args.value(1).trimmed();
             if (paramOption.appId.isEmpty()) {
                 parser.showHelp(-1);
                 return -1;
             }
             paramOption.force = parser.isSet(optNoCache);
             paramOption.repoPoint = repoType;
             paramOption.appId = args.value(1);

             QDBusPendingReply<linglong::service::QueryReply> dbusReply = packageManager.Query(paramOption);
             dbusReply.waitForFinished();
             linglong::service::QueryReply reply = dbusReply.value();

             auto appMetaInfoList = util::arrayFromJson<package::AppMetaInfoList>(reply.result);

             if (1 == appMetaInfoList.size() && "flatpakquery" == appMetaInfoList.at(0)->appId) {
                 printFlatpakAppInfo(appMetaInfoList);
             } else {
                 printAppInfo(appMetaInfoList);
             }
             return 0;
         }},
        {"uninstall", // 卸载玲珑包
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("uninstall", "uninstall an application", "uninstall");
             addRefArguments(parser, {optChannel, optModule});

             auto optAllVer = QCommandLineOption("all-version", "uninstall all version application", "");
             parser.addOption(optAllVer);

             auto optDelData = QCommandLineOption("delete-data", "delete app data", "");
             parser.addOption(optDelData);

             parser.process(app);

             args = parser.positionalArguments();
             auto repoType = parser.value(optRepoPoint);
             if (args.size() != 2 || (!repoType.isEmpty() && "flatpak" != repoType)) {
                 parser.showHelp(-1);
                 return -1;
             }

             auto appInfo = args.value(1);
             packageManager.setTimeout(1000 * 60 * 60 * 24);
             QDBusPendingReply<linglong::service::Reply> dbusReply;
             linglong::service::UninstallParamOption paramOption;
             // appId format: org.deepin.calculator/1.2.6 in multi-version
             QStringList appInfoList = appInfo.split("/");
             paramOption.appId = appInfo;
             if (appInfoList.size() > 1) {
                 paramOption.appId = appInfoList.at(0);
                 paramOption.version = appInfoList.at(1);
             }

             paramOption.channel = parser.value(optChannel);
             paramOption.appModule = parser.value(optModule);
             paramOption.delAppData = parser.isSet(optDelData);
             paramOption.repoPoint = repoType;
             linglong::service::Reply reply;
             qInfo().noquote() << "uninstall" << appInfo << ", please wait a few minutes...";
             paramOption.delAllVersion = parser.isSet(optAllVer);
             dbusReply = packageManager.Uninstall(paramOption);
             dbusReply.waitForFinished();
             reply = dbusReply.value();

             if (reply.code != STATUS_CODE(kPkgUninstallSuccess)) {
                 qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                 return -1;
             }
             qInfo().noquote() << "message:" << reply.message;
             return 0;
         }},
        {"list", // 查询已安装玲珑包
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("list", "show installed package", "list");
             parser.process(app);

             QDBusPendingReply<QVariantMapList> dbusReply = packageManager.List({});
             dbusReply.waitForFinished();
             if (dbusReply.isError()) {
                 qCritical() << dbusReply.error();
                 return dbusReply.error().type();
             }

             auto list = fromVariantList<package::AppMetaInfoList>(toVariant(dbusReply.value()));
             printAppInfo(list);

             return 0;
         }},
        {"repo",
         [&](QCommandLineParser &parser) -> int {
             parser.clearPositionalArguments();
             parser.addPositionalArgument("repo", "config remote repo", "repo");
             parser.addPositionalArgument("modify", "modify the url of repo", "modify");
             parser.addPositionalArgument("url", "the url of repo", "");
             parser.process(app);
             args = parser.positionalArguments();
             auto subCmd = args.value(1).trimmed();
             auto url = args.value(2).trimmed();
             if (url.isEmpty() || "modify" != subCmd || args.size() != 3) {
                 parser.showHelp(-1);
                 return -1;
             }
             QDBusPendingReply<linglong::service::Reply> dbusReply = packageManager.ModifyRepo(url);
             dbusReply.waitForFinished();
             linglong::service::Reply reply = dbusReply.value();
             if (reply.code != STATUS_CODE(kErrorModifyRepoSuccess)) {
                 qCritical().noquote() << "message:" << reply.message << ", errcode:" << reply.code;
                 return -1;
             }
             qInfo().noquote() << reply.message;
             return 0;
         }},
    };

    if (subcommandMap.contains(command)) {
        auto subcommand = subcommandMap[command];
        return subcommand(parser);
    } else {
        parser.showHelp();
    }
}
