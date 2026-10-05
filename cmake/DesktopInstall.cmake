# Qt 6.4 cannot deploy Linux runtime libraries. That package deliberately uses
# Ubuntu's system Qt, verified by a separate ELF/QML/package inventory.
set(CMAKE_INSTALL_BINDIR ".")
set(CMAKE_INSTALL_LIBDIR ".")
install(TARGETS anteater-chess RUNTIME DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/COPYRIGHT" "${CMAKE_SOURCE_DIR}/VERSION" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/tools/packaging/templates/README.md" DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/docs/user" "${CMAKE_SOURCE_DIR}/docs/legacy" DESTINATION docs)
if(WIN32)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        message(FATAL_ERROR "Runtime distribution accepts Release only; Debug is never redistributed.")
    endif()
    file(TO_CMAKE_PATH "$ENV{VCToolsRedistDir}" AC_REDIST_ROOT)
    set(AC_REDIST_DIR "${AC_REDIST_ROOT}/x64/Microsoft.VC143.CRT")
    if(NOT AC_REDIST_ROOT MATCHES "/14[.]44[.][0-9]+/?$" OR NOT EXISTS "${AC_REDIST_DIR}/vcruntime140.dll")
        message(FATAL_ERROR "A verified v143 14.44 Release REDIST source is required.")
    endif()
    qt_generate_deploy_qml_app_script(TARGET anteater-chess OUTPUT_SCRIPT ac_deploy
        NO_COMPILER_RUNTIME NO_TRANSLATIONS DEPLOY_TOOL_OPTIONS --no-patchqt --no-system-d3d-compiler)
    install(CODE "set(QT_DEPLOY_BIN_DIR .)\nset(QT_DEPLOY_LIB_DIR .)")
    install(SCRIPT "${ac_deploy}")
    install(DIRECTORY "${AC_REDIST_DIR}/" DESTINATION . FILES_MATCHING PATTERN "*.dll")
    set(ac_install_platform windows)
else()
    if(NOT Qt6_VERSION VERSION_EQUAL "6.4.2")
        message(FATAL_ERROR "The maintained Linux distribution requires system Qt 6.4.2.")
    endif()
    qt_generate_deploy_qml_app_script(TARGET anteater-chess FILENAME_VARIABLE ac_deploy
        NO_UNSUPPORTED_PLATFORM_ERROR DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM)
    # Qt 6.4 emits an unquoted include in its generated deployment script.
    # Invoke the public QML deployment API with a quoted support path instead.
    configure_file("${CMAKE_SOURCE_DIR}/cmake/SystemQtInstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/SystemQtInstall.cmake" @ONLY)
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/SystemQtInstall.cmake")
    set(ac_install_platform linux)
endif()
install(FILES "${CMAKE_SOURCE_DIR}/tools/packaging/templates/INSTALL-${ac_install_platform}.md"
    DESTINATION . RENAME INSTALL.md)
get_target_property(ac_qmake Qt6::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${ac_qmake}" -query QT_INSTALL_PREFIX OUTPUT_VARIABLE AC_QT_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
configure_file("${CMAKE_SOURCE_DIR}/cmake/RuntimeReceipt.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/RuntimeReceipt.cmake" @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/RuntimeReceipt.cmake")
