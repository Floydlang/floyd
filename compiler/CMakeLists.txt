# Generated on 2018-9-21 20:42:18 by version CURRENT_PROJECT_VERSION of mulle-xcode-to-cmake
# Command line:
#    mulle-xcode-to-cmake -b export floyd_speak.xcodeproj

# For emscripten, fix CMAKE_CXX_FLAGS, then try
# cmake -DCMAKE_TOOLCHAIN_FILE=/usr/lib/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -G  "Unix Makefiles" ..
# https://llvm.org/docs/CMake.html

project( floyd_speak)

cmake_minimum_required (VERSION 3.4)

IF(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc ")
ENDIF(MSVC)
#add_subdirectory(libs/Celero-master)

#SET (CMAKE_BUILD_TYPE Release)
SET (CMAKE_BUILD_TYPE Debug)

include_directories(/usr/local/Cellar/llvm@8/8.0.1/include 
/usr/lib/llvm-8/include passes parts target_tool  /usr/include target_benchmark_internals libs/benchmark/include libs/benchmark/googletest/googlemock/ libs/benchmark/googletest/googlemock/include libs/benchmark/googletest/googletest/include/ floyd_runtime floyd_basics floyd_ast bytecode_interpreter  parts/immer-master floyd_parser parts/sha1 llvm_pipeline  .)

if (MSVC)  # - Microsoft Visual C++
set (PROJECTNAME_ARCHITECTURE "x64")
endif()
#message("WTF")

if (MSVC)  # - Microsoft Visual C++
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"                                            
    CACHE STRING "")
    message(" [INFO] VCPKG CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
	set (VCPKG_ROOT ENV{VCPKG_ROOT})
else()
  #set (VCPKG_ROOT "C:/work/FloydLang/vcpkg")
  set (VCPKG_ROOT "${CMAKE_SOURCE_DIR}/../../../vcpkg/")
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../../../vcpkg/scripts/buildsystems/vcpkg.cmake"                                            
    CACHE STRING "")
  #set(CMAKE_TOOLCHAIN_FILE "C:/work/FloydLang/vcpkg/scripts/buildsystems/vcpkg.cmake"                                            
  #  CACHE STRING "")

    message(" [INFO] VCPKG CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()

find_package(Threads REQUIRED)
find_package(LLVM  REQUIRED PATHS  "${CMAKE_SOURCE_DIR}/../../../vcpkg/packages/llvm_x64-windows/share/llvm/")
#find_package(readline REQUIRED)


endif(MSVC)

#message(" [INFO] VCPKG CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
#find_package(LLVM  REQUIRED) 

# TODO, use find()
include_directories(${VCPKG_ROOT}/packages/llvm_x64-windows/include)
include_directories(${VCPKG_ROOT}/packages/readline-win32_x64-windows/include)
include_directories(${VCPKG_ROOT}/packages/pthreads_x64-windows/include)
include_directories(${VCPKG_ROOT}/packages/dirent_x64-windows/include)
include_directories(${VCPKG_ROOT}/packages/getopt-win32_x64-windows/include) 
#message(${VCPKG_ROOT}/packages/llvm_x64-windows/include)
 
# for newer cmake 
#add_compile_definitions(DEBUG)
add_definitions(-DDEBUG)

#add_definitions(-DQUARK_ASSERT_ON true)
#add_definitions(-DQUARK_TRACE_ON true)
#add_definitions(-DQUARK_UNIT_TESTS_ON true)

# -fno-exceptions
set(CMAKE_CXX_FLAGS "-D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS  -std=c++17")

#Options for emscripten
#set(CMAKE_CXX_FLAGS "--closure 1 -std=c++1z -s USE_PTHREADS=1 -Os -s WASM=0 -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0 --bind")


set(FLOYD_SOURCES
bytecode_interpreter/bytecode_corelib.cpp
bytecode_interpreter/bytecode_generator.cpp
bytecode_interpreter/bytecode_helpers.cpp
bytecode_interpreter/bytecode_interpreter.cpp
bytecode_interpreter/bytecode_intrinsics.cpp
bytecode_interpreter/floyd_interpreter.cpp
cpp_experiments.cpp
#examples/benchmarks.floyd
#examples/game_of_life.cpp
#examples/process_test1.floyd
#examples/test_main.floyd
floyd_ast/ast.cpp
floyd_ast/ast_visitor.cpp
floyd_ast/expression.cpp
floyd_ast/statement.cpp
floyd_basics/ast_value.cpp
floyd_basics/compiler_basics.cpp
floyd_basics/compiler_helpers.cpp
floyd_basics/type_interner.cpp
floyd_basics/typeid.cpp
floyd_parser/floyd_parser.cpp
floyd_parser/floyd_syntax.cpp
floyd_parser/parse_expression.cpp
floyd_parser/parse_statement.cpp
floyd_parser/parser_primitives.cpp
floyd_runtime/floyd_corelib.cpp
floyd_runtime/floyd_runtime.cpp
floyd_runtime/quadratic_probing_hash_table.cpp
floyd_runtime/value_backend.cpp
floyd_runtime/value_features.cpp
floyd_runtime/value_thunking.cpp
floyd_runtime/variable_length_quantity.cpp
floyd_runtime/write_cache.cpp
language_validations/floyd_test_suite.cpp
language_validations/issue_regression_tests.cpp
language_validations/test_helpers.cpp
#libs/benchmark/googletest/googlemock/src/gmock-all.cc
#libs/benchmark/googletest/googletest/src/gtest-all.cc
libs/benchmark/src/benchmark.cc
libs/benchmark/src/benchmark_api_internal.cc
libs/benchmark/src/benchmark_register.cc
libs/benchmark/src/benchmark_runner.cc
libs/benchmark/src/colorprint.cc
libs/benchmark/src/commandlineflags.cc
libs/benchmark/src/complexity.cc
libs/benchmark/src/console_reporter.cc
libs/benchmark/src/counter.cc
libs/benchmark/src/csv_reporter.cc
libs/benchmark/src/json_reporter.cc
libs/benchmark/src/reporter.cc
libs/benchmark/src/sleep.cc
libs/benchmark/src/statistics.cc
libs/benchmark/src/string_util.cc
libs/benchmark/src/sysinfo.cc
libs/benchmark/src/timers.cc
llvm_pipeline/floyd_llvm.cpp
llvm_pipeline/floyd_llvm_codegen.cpp
llvm_pipeline/floyd_llvm_codegen_basics.cpp
llvm_pipeline/floyd_llvm_corelib.cpp
llvm_pipeline/floyd_llvm_helpers.cpp
llvm_pipeline/floyd_llvm_intrinsics.cpp
llvm_pipeline/floyd_llvm_optimization.cpp
llvm_pipeline/floyd_llvm_runtime.cpp
llvm_pipeline/floyd_llvm_runtime_functions.cpp
llvm_pipeline/floyd_llvm_types.cpp
parts/command_line_parser.cpp
parts/file_handling.cpp
parts/hardware_caps.cpp
parts/immutable_ref_value.cpp
parts/json_support.cpp
parts/os_process.cpp
parts/quark.cpp
parts/sha1/sha1.cpp
parts/sha1_class.cpp
parts/text_parser.cpp
parts/utils.cpp
passes/ast_helpers.cpp
passes/collect_used_types.cpp
passes/desugar_pass.cpp
passes/parse_tree_to_ast_conv.cpp
passes/semantic_analyser.cpp
semantic_ast.cpp
software_system.cpp
target_benchmark_internals/benchmark_basics.cpp
target_benchmark_internals/compressed_vector_benchmark.cpp
target_benchmark_internals/interpretator_benchmark.cpp
target_tool/floyd_command_line_parser.cpp
target_tool/floyd_main.cpp
target_tool/floyd_repl.cpp
target_tool/format_table.cpp
)

if (MSVC)  # - Microsoft Visual C++
set(FLOYD_SOURCES
${FLOYD_SOURCES}
parts/hardware_caps_windows.cpp
)
elseif(APPLE)
set(FLOYD_SOURCES
${FLOYD_SOURCES}
parts/hardware_caps_macos.cpp
)
else()
set(FLOYD_SOURCES
${FLOYD_SOURCES}
parts/hardware_caps_linux.cpp
parts/hardware_cacheinfo.cpp
parts/cpu_features.cpp
)
endif()

# llvm-config --cxxflags --ldflags --system-libs --libs engine interpreter
#
# -I/usr/include -std=c++17  -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
# -L/usr/lib
# -lLLVM-8
#
##
## floyd_speak Files
##

# uncomment this for mulle-objc to search libraries first
# set( CMAKE_FIND_FRAMEWORK "LAST")

set( FLOYD_SPEAK_SOURCES
${FLOYD_SOURCES}
)

set( FLOYD_SPEAK_STATIC_DEPENDENCIES
)


set( FLOYD_SPEAK_DEPENDENCIES
)

set( BENCHMARK_SOURCES
bytecode_interpreter/bytecode_corelib.cpp
bytecode_interpreter/bytecode_generator.cpp
bytecode_interpreter/bytecode_helpers.cpp
bytecode_interpreter/bytecode_interpreter.cpp
bytecode_interpreter/bytecode_intrinsics.cpp
bytecode_interpreter/floyd_interpreter.cpp
cpp_experiments.cpp
examples/process_test1.floyd
examples/test_main.floyd
floyd_ast/ast.cpp
floyd_ast/ast_visitor.cpp
floyd_ast/expression.cpp
floyd_ast/statement.cpp
floyd_basics/ast_value.cpp
floyd_basics/compiler_basics.cpp
floyd_basics/compiler_helpers.cpp
floyd_basics/typeid.cpp
floyd_parser/floyd_parser.cpp
floyd_parser/floyd_syntax.cpp
floyd_parser/parse_expression.cpp
floyd_parser/parse_statement.cpp
floyd_parser/parser_primitives.cpp
floyd_runtime/floyd_corelib.cpp
floyd_runtime/floyd_runtime.cpp
floyd_runtime/quadratic_probing_hash_table.cpp
floyd_runtime/value_backend.cpp
floyd_runtime/value_features.cpp
floyd_runtime/value_thunking.cpp
floyd_runtime/variable_length_quantity.cpp
language_validations/floyd_test_suite.cpp
language_validations/issue_regression_tests.cpp
language_validations/test_helpers.cpp
libs/benchmark/googletest/googlemock/src/gmock-all.cc
libs/benchmark/googletest/googletest/src/gtest-all.cc
libs/benchmark/src/benchmark.cc
libs/benchmark/src/benchmark_api_internal.cc
libs/benchmark/src/benchmark_register.cc
libs/benchmark/src/benchmark_runner.cc
libs/benchmark/src/colorprint.cc
libs/benchmark/src/commandlineflags.cc
libs/benchmark/src/complexity.cc
libs/benchmark/src/console_reporter.cc
libs/benchmark/src/counter.cc
libs/benchmark/src/csv_reporter.cc
libs/benchmark/src/json_reporter.cc
libs/benchmark/src/reporter.cc
libs/benchmark/src/sleep.cc
libs/benchmark/src/statistics.cc
libs/benchmark/src/string_util.cc
libs/benchmark/src/sysinfo.cc
libs/benchmark/src/timers.cc
llvm_pipeline/floyd_llvm.cpp
llvm_pipeline/floyd_llvm_codegen.cpp
llvm_pipeline/floyd_llvm_corelib.cpp
llvm_pipeline/floyd_llvm_helpers.cpp
llvm_pipeline/floyd_llvm_runtime.cpp
llvm_pipeline/floyd_llvm_types.cpp
parts/command_line_parser.cpp
parts/file_handling.cpp
parts/hardware_caps.cpp
parts/hardware_caps_macos.cpp
parts/immutable_ref_value.cpp
parts/json_support.cpp
parts/os_process.cpp
parts/quark.cpp
parts/sha1/sha1.cpp
parts/sha1_class.cpp
parts/text_parser.cpp
parts/utils.cpp
passes/ast_helpers.cpp
passes/collect_used_types.cpp
passes/desugar_pass.cpp
passes/parse_tree_to_ast_conv.cpp
passes/semantic_analyser.cpp
semantic_ast.cpp
software_system.cpp
target_benchmark_internals/benchmark_basics.cpp
target_benchmark_internals/benchmark_soundsystem.cpp
target_benchmark_internals/compressed_vector_benchmark.cpp
target_benchmark_internals/floyd_benchmark_main.cpp
target_benchmark_internals/interpretator_benchmark.cpp
target_tool/format_table.cpp
)

##
## floyd_speak UT Files
##

set( FLOYD_SPEAK__UT_SOURCES
${FLOYD_SOURCES}
)

set( FLOYD_SPEAK__UT_STATIC_DEPENDENCIES
)


set( FLOYD_SPEAK__UT_DEPENDENCIES
)


##
## floyd_speak
##

add_executable( floyd MACOSX_BUNDLE
${FLOYD_SPEAK_SOURCES}
${FLOYD_SPEAK_PUBLIC_HEADERS}
${FLOYD_SPEAK_PROJECT_HEADERS}
${FLOYD_SPEAK_PRIVATE_HEADERS}
${FLOYD_SPEAK_RESOURCES}
)

target_include_directories( floyd
   PUBLIC
)


if(MSVC)   

  link_directories("${CMAKE_SOURCE_DIR}/../../../vcpkg/installed/x64-windows/debug/lib/")
  #find_library(READLINE_LIBRARY readline)

  target_link_libraries( floyd
  ${FLOYD_SPEAK_STATIC_DEPENDENCIES}
  ${FLOYD_SPEAK_DEPENDENCIES} ${CMAKE_THREAD_LIBS_INIT} Threads::Threads  LLVMMC LLVMLTO Shlwapi)
  # ${CMAKE_SOURCE_DIR}/../../../vcpkg/installed/x64-windows/debug/lib/readline.lib
elseif(APPLE)
target_link_options(floyd  PRIVATE "-L/usr/local/opt/llvm@8/lib -Wl,-rpath,/usr/local/opt/llvm@8/lib")
target_link_libraries( floyd
${FLOYD_SPEAK_STATIC_DEPENDENCIES}
${FLOYD_SPEAK_DEPENDENCIES} pthread LLVM ${LLVM_LIB} readline)

else()
target_link_libraries( floyd
${FLOYD_SPEAK_STATIC_DEPENDENCIES}
${FLOYD_SPEAK_DEPENDENCIES} pthread LLVM-8 ${LLVM_LIB} readline)
ENDIF()


##
## floyd_speak UT
##


#add_executable( floydut MACOSX_BUNDLE
#${FLOYD_SOURCES}
#${FLOYD_SPEAK__UT_SOURCES}
#${FLOYD_SPEAK__UT_PUBLIC_HEADERS}
#${FLOYD_SPEAK__UT_PROJECT_HEADERS}
#${FLOYD_SPEAK__UT_PRIVATE_HEADERS}
#${FLOYD_SPEAK__UT_RESOURCES}
#)

#target_compile_definitions(floydut PRIVATE DEBUG=1)

#target_include_directories( floydut
#   PUBLIC
#)

#target_link_libraries( floydut
#${FLOYD_SPEAK__UT_STATIC_DEPENDENCIES}
#${FLOYD_SPEAK__UT_DEPENDENCIES} pthread LLVM-8 readline
#)
IF(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")
	add_compile_options("/std:c++latest")
	add_definitions(-D_USE_MATH_DEFINES -D BENCHMARK_OS_WINDOWS)	 
	target_compile_features(floyd PUBLIC cxx_std_20)
ELSE(MSVC)
set_property(TARGET floyd PROPERTY CXX_STANDARD 17)
set_property(TARGET floyd PROPERTY CXX_STANDARD_REQUIRED ON)
ENDIF(MSVC)

#The package llvm:x64-windows provides CMake targets:

#   find_package(Clang CONFIG REQUIRED)
    # Note: 27 target(s) were omitted.
#    target_link_libraries(main PRIVATE clangAST clangLex libclang clangEdit)

#    find_package(LLVM CONFIG REQUIRED)
    # Note: 62 target(s) were omitted.
#    target_link_libraries(main PRIVATE LTO LLVMMC ${name} LLVMLTO)

#set_property(TARGET floydut PROPERTY CXX_STANDARD 17)


##
## benchmark
##

# Does not currently compile
#add_executable( benchmark MACOSX_BUNDLE
#${BENCHMARK_SOURCES}
#)
