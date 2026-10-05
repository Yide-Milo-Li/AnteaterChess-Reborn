# CMake's AUTO decoding can interpret UTF-8 /showIncludes output using the
# system ANSI page on a localized Windows host. Detect in Ninja's own encoding
# so its byte comparison records real header dependencies.
execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" -t wincodepage
    OUTPUT_VARIABLE ac_ninja_encoding RESULT_VARIABLE ac_ninja_encoding_exit)
if(NOT ac_ninja_encoding_exit EQUAL 0)
    message(FATAL_ERROR "Cannot determine native Ninja's dependency encoding")
endif()
if(ac_ninja_encoding MATCHES "UTF-8")
    set(ac_probe_dir "${CMAKE_BINARY_DIR}/CMakeFiles/NativeDependencyProbe")
    file(MAKE_DIRECTORY "${ac_probe_dir}")
    file(WRITE "${ac_probe_dir}/ac-dependency-probe.hpp" "#pragma once\n")
    file(WRITE "${ac_probe_dir}/probe.cpp" "#include \"ac-dependency-probe.hpp\"\nint probe;\n")
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /c probe.cpp
        WORKING_DIRECTORY "${ac_probe_dir}" ENCODING UTF-8
        OUTPUT_VARIABLE ac_include_output ERROR_VARIABLE ac_include_error
        RESULT_VARIABLE ac_include_exit)
    if(NOT ac_include_exit EQUAL 0)
        message(FATAL_ERROR "Native dependency probe failed: ${ac_include_error}")
    endif()
    string(REGEX MATCH "(^|\n)([^\n]+: +)[A-Za-z]:[/\\][^\n]*ac-dependency-probe.hpp"
        ac_include_match "${ac_include_output}")
    if(NOT ac_include_match)
        message(FATAL_ERROR "Cannot detect native compiler's /showIncludes prefix")
    endif()
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_MATCH_2}")
endif()
