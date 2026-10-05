find_package(Qt6 6.4.2 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg)
if(WIN32 AND NOT Qt6_VERSION VERSION_EQUAL "6.11.2")
    message(FATAL_ERROR "Windows requires the official Qt 6.11.2 MSVC 2022 x64 kit.")
endif()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
add_library(ac_desktop STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime.cpp ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/board_model.cpp ${CMAKE_CURRENT_SOURCE_DIR}/models/board_model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/history_model.cpp ${CMAKE_CURRENT_SOURCE_DIR}/models/history_model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/game_enums.hpp ${CMAKE_CURRENT_SOURCE_DIR}/models/settings_draft.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/input_model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/clock_model.cpp ${CMAKE_CURRENT_SOURCE_DIR}/models/clock_model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/models/status_model.cpp ${CMAKE_CURRENT_SOURCE_DIR}/models/status_model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/async/search_jobs.cpp ${CMAKE_CURRENT_SOURCE_DIR}/async/search_jobs.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/app/application_controller.cpp ${CMAKE_CURRENT_SOURCE_DIR}/app/application_controller.hpp)
target_include_directories(ac_desktop PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/app ${CMAKE_CURRENT_SOURCE_DIR}/models ${CMAKE_CURRENT_SOURCE_DIR}/runtime ${CMAKE_CURRENT_SOURCE_DIR}/async)
target_link_libraries(ac_desktop PUBLIC ac_session ac_ai Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Svg)
set(ac_qml_files ${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/SetupPage.qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/GameplayPage.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/Board.qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/Theme.qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/Panel.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/MoveField.qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/ActionButton.qml)
foreach(ac_qml_file IN LISTS ac_qml_files)
    get_filename_component(ac_qml_name "${ac_qml_file}" NAME)
    set_source_files_properties("${ac_qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${ac_qml_name}")
endforeach()
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(ac_desktop URI AnteaterChess.Reborn VERSION 1.0
    CLASS_NAME AnteaterChessRebornPlugin RESOURCE_PREFIX /qt/qml
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml/AnteaterChess/Reborn" QML_FILES ${ac_qml_files})
add_library(ac_qml_import OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/qml_import.cpp)
target_link_libraries(ac_qml_import PRIVATE Qt6::Qml)
add_library(ac_resources OBJECT "${CMAKE_SOURCE_DIR}/assets/resources.qrc")
target_link_libraries(ac_resources PRIVATE Qt6::Core)
qt_add_executable(anteater-chess ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp $<TARGET_OBJECTS:ac_resources> $<TARGET_OBJECTS:ac_qml_import>)
target_link_libraries(anteater-chess PRIVATE ac_desktop ac_desktopplugin Qt6::QuickControls2)
set_target_properties(anteater-chess PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(WIN32 AND NOT AC_WINDOWS_CONSOLE)
    set_target_properties(anteater-chess PROPERTIES WIN32_EXECUTABLE ON)
endif()
if(AC_BUILD_DISTRIBUTION)
    include(${CMAKE_SOURCE_DIR}/cmake/DesktopInstall.cmake)
endif()
if(BUILD_TESTING)
find_package(Qt6 6.4.2 REQUIRED COMPONENTS Test QuickTest)
foreach(name runtime desktop)
    add_executable(test_qt_${name} ${CMAKE_SOURCE_DIR}/tests/qt/test_${name}.cpp $<TARGET_OBJECTS:ac_resources> $<TARGET_OBJECTS:ac_qml_import>)
    target_sources(test_qt_${name} PRIVATE $<TARGET_OBJECTS:ac_test_diagnostics>)
    target_link_libraries(test_qt_${name} PRIVATE ac_desktop ac_desktopplugin Qt6::Test Qt6::QuickControls2)
    set_target_properties(test_qt_${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/qt")
    add_test(NAME test_qt_${name} COMMAND test_qt_${name} -o -,txt)
    set_tests_properties(test_qt_${name} PROPERTIES TIMEOUT 90)
    if(name STREQUAL "runtime")
        set_tests_properties(test_qt_${name} PROPERTIES LABELS platform)
    else()
        set_tests_properties(test_qt_${name} PROPERTIES LABELS gui)
    endif()
endforeach()
add_executable(test_qml ${CMAKE_SOURCE_DIR}/tests/qt/test_qml.cpp $<TARGET_OBJECTS:ac_resources> $<TARGET_OBJECTS:ac_qml_import>)
target_sources(test_qml PRIVATE $<TARGET_OBJECTS:ac_test_diagnostics>)
target_link_libraries(test_qml PRIVATE ac_desktop ac_desktopplugin Qt6::QuickTest Qt6::QuickControls2)
target_compile_definitions(test_qml PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}/tests/qt/qml")
add_test(NAME test_qml COMMAND test_qml -o -,txt)
set_tests_properties(test_qml PROPERTIES LABELS gui ENVIRONMENT "QT_QUICK_CONTROLS_STYLE=Basic")
endif()
