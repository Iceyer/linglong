find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
# QT sqlite 支持
find_package(
  Qt5
  COMPONENTS Concurrent Sql
  REQUIRED
)
pkg_search_module(
  OSTREE1
  REQUIRED
  ostree-1
)

set(LL_SERVICE_SOURCES
    # cmake-format: sortable
    ../module/flatpak/flatpak_manager.cpp
    ../module/package/info.cpp
    ../module/package/package.cpp
    ../module/repo/ostree_repo.cpp
    ../module/repo/ostree_repo.h
    ../module/repo/ostree_repohelper.cpp
    ../module/repo/repo.cpp
    ../module/repo/repo_client.cpp
    ../module/repo/repo_client.h
    ../module/runtime/app.cpp
    ../module/runtime/container.cpp
    ../module/runtime/oci.h
    ../module/util/config/config.h
    ../module/util/connection.cpp
    ../module/util/desktop_entry.cpp
    ../module/util/file.cpp
    ../module/util/http/http_client.cpp
    ../module/util/http/http_client.h
    ../module/util/httpclient.cpp
    ../module/util/json.h
    ../module/util/log_handler.cpp
    ../module/util/sysinfo.cpp
    ../module/util/uuid.cpp
    ../module/util/xdg.cpp
    ../package_manager/impl/app_status.cpp
    ../package_manager/impl/appinfo_cache.cpp
    ../package_manager/impl/service_dbus_common.h
    impl/app_manager.cpp
    impl/param_option.h
    impl/reply.h
    main.cpp
    resource/resource.qrc
    ../module/util/config/config.cpp)

qt5_add_dbus_adaptor(
  LL_SERVICE_SOURCES
  ../module/dbus_ipc/org.deepin.linglong.AppManager.xml
  impl/app_manager.h
  linglong::service::AppManager
  dbus_gen_app_manager_adaptor
)

set(LINK_LIBS
    ${LINK_LIBS}
    CURL::libcurl
    ${OSTREE1_LDFLAGS}
    Qt5::Sql
)

add_executable(ll-service ${LL_SERVICE_SOURCES})

target_include_directories(ll-service PRIVATE ${OSTREE1_INCLUDE_DIRS})

target_link_libraries(ll-service PRIVATE ${LINK_LIBS})

install(TARGETS ll-service RUNTIME DESTINATION bin)

install(FILES misc/config.yaml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/linglong/)

install(FILES misc/xdg/org.deepin.linglong.service.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
        )

install(FILES misc/xdg/org.deepin.linglong.service.desktop
        DESTINATION /etc/xdg/autostart
        )

set(PERMISSION
    # cmake-format: sortable
    misc/linglong/calendar.json
    misc/linglong/camera.json
    misc/linglong/hostdir.json
    misc/linglong/mic.json
    misc/linglong/screenshot.json
    misc/linglong/userdir.json
)

install(FILES ${PERMISSION}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/permission/policy/linglong
)

install(FILES misc/xdg/linglong.xml
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/packages
        )

install(FILES ${CMAKE_SOURCE_DIR}/scripts/linglong.sh
        DESTINATION /etc/profile.d
        )

install(
    FILES ${CMAKE_SOURCE_DIR}/scripts/linglong.sh
    DESTINATION /etc/X11/Xsession.d/
    RENAME 21linglong
)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/50-linglong
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user-environment-generators/
        )
