# Keep Qt's public import/deployment graph and its installed binaries intact.
# Only Unicode Windows source/build paths need the response-file adapter.
if(WIN32 AND "${CMAKE_SOURCE_DIR};${CMAKE_BINARY_DIR}" MATCHES "[^ -~]")
    get_target_property(AC_QML_SCANNER Qt6::qmlimportscanner IMPORTED_LOCATION)
    if(NOT AC_QML_SCANNER)
        foreach(ac_config RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
            get_target_property(AC_QML_SCANNER Qt6::qmlimportscanner IMPORTED_LOCATION_${ac_config})
            if(AC_QML_SCANNER)
                break()
            endif()
        endforeach()
    endif()
    if(NOT EXISTS "${AC_QML_SCANNER}")
        message(FATAL_ERROR "The official Qt QML scanner is missing.")
    endif()
    set(ac_scanner_dir "${CMAKE_BINARY_DIR}/qt-utf8-scanner")
    file(MAKE_DIRECTORY "${ac_scanner_dir}")
    configure_file("${CMAKE_SOURCE_DIR}/cmake/QtUtf8Scanner.cpp.in" "${ac_scanner_dir}/scanner.cpp" @ONLY)
    try_compile(ac_scanner_compiled "${ac_scanner_dir}/bootstrap"
        "${ac_scanner_dir}/scanner.cpp" CXX_STANDARD 20
        CMAKE_FLAGS "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded"
        COPY_FILE "${ac_scanner_dir}/scanner.exe" OUTPUT_VARIABLE ac_scanner_output)
    file(WRITE "${ac_scanner_dir}/compile.txt" "${ac_scanner_output}")
    if(NOT ac_scanner_compiled)
        message(FATAL_ERROR "QML UTF-8 scanner adapter failed: ${ac_scanner_output}")
    endif()
    set_target_properties(Qt6::qmlimportscanner PROPERTIES IMPORTED_LOCATION "${ac_scanner_dir}/scanner.exe")
    message(STATUS "QML import scanner: UTF-8 response adapter for Unicode Windows paths")
endif()
