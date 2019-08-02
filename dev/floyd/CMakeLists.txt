# Generated on 2018-9-21 20:42:18 by version CURRENT_PROJECT_VERSION of mulle-xcode-to-cmake
# Command line:
#    mulle-xcode-to-cmake -b export floyd_speak.xcodeproj

# For emscripten, fix CMAKE_CXX_FLAGS, then try
# cmake -DCMAKE_TOOLCHAIN_FILE=/usr/lib/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -G  "Unix Makefiles" ..

project( floyd_speak)

cmake_minimum_required (VERSION 3.4)

IF(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")
ENDIF(MSVC)
#add_subdirectory(libs/Celero-master)

#SET (CMAKE_BUILD_TYPE Release)
SET (CMAKE_BUILD_TYPE Debug)

include_directories(parts /usr/include libs/benchmark/include libs/benchmark/googletest/googlemock/ libs/benchmark/googletest/googlemock/include floyd_runtime floyd_basics floyd_ast bytecode_interpreter  parts/immer-master floyd_parser parts/sha1 llvm_pipeline  .)

# for newer cmake 
#add_compile_definitions(DEBUG)
add_definitions(-DDEBUG)

#add_definitions(-DQUARK_ASSERT_ON true)
#add_definitions(-DQUARK_TRACE_ON true)
#add_definitions(-DQUARK_UNIT_TESTS_ON true)

# -fno-exceptions
set(CMAKE_CXX_FLAGS "-std=c++17 -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")

#Options for emscripten
#set(CMAKE_CXX_FLAGS "--closure 1 -std=c++1z -s USE_PTHREADS=1 -Os -s WASM=0 -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0 --bind")

set( FLOYD_SOURCES
benchmark_basics.cpp
bytecode_interpreter/bytecode_corecalls.cpp
bytecode_interpreter/bytecode_corelib.cpp
bytecode_interpreter/bytecode_generator.cpp
bytecode_interpreter/bytecode_helpers.cpp
bytecode_interpreter/bytecode_interpreter.cpp
bytecode_interpreter/floyd_interpreter.cpp
collect_used_types.cpp
compiler_helpers.cpp
cpp_experiments.cpp
desugar_pass.cpp
#floyd/examples/game_of_life.cpp
#floyd/examples/process_test1.floyd
#floyd/examples/test_main.floyd
floyd_ast/ast.cpp
floyd_ast/ast_helpers.cpp
floyd_ast/ast_visitor.cpp
floyd_ast/expression.cpp
floyd_ast/statement.cpp
floyd_basics/ast_value.cpp
floyd_basics/compiler_basics.cpp
floyd_basics/typeid.cpp
floyd_main.cpp
floyd_parser/floyd_parser.cpp
floyd_parser/floyd_syntax.cpp
floyd_parser/parse_expression.cpp
floyd_parser/parse_statement.cpp
floyd_parser/parser_primitives.cpp
floyd_repl.cpp
floyd_runtime/floyd_corelib.cpp
floyd_runtime/floyd_runtime.cpp
floyd_runtime/variable_length_quantity.cpp
floyd_test_suite.cpp
interpretator_benchmark.cpp
issue_regression_tests.cpp
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
llvm_pipeline/floyd_llvm_codegen.cpp
llvm_pipeline/floyd_llvm_corelib.cpp
llvm_pipeline/floyd_llvm_helpers.cpp
llvm_pipeline/floyd_llvm_runtime.cpp
llvm_pipeline/floyd_llvm_types.cpp
parse_tree_to_ast_conv.cpp
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
semantic_analyser.cpp
software_system.cpp
test_helpers.cpp
)

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
bytecode_interpreter/bytecode_helpers.cpp
collect_used_types.cpp
compiler_helpers.cpp
compressed_vector_benchmark.cpp
examples/process_test1.floyd
examples/test_main.floyd
floyd_ast/ast_helpers.cpp
floyd_benchmark_main.cpp
floyd_runtime/floyd_runtime.cpp
floyd_runtime/variable_length_quantity.cpp
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
llvm_pipeline/floyd_llvm_codegen.cpp
llvm_pipeline/floyd_llvm_corelib.cpp
parse_tree_to_ast_conv.cpp
parts/hardware_caps.cpp
parts/os_process.cpp
parts/quark.cpp
parts/sha1/sha1.cpp
parts/utils.cpp
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



target_link_libraries( floyd
${FLOYD_SPEAK_STATIC_DEPENDENCIES}
${FLOYD_SPEAK_DEPENDENCIES} pthread LLVM-8 readline
)


##
## floyd_speak UT
##


add_executable( floydut MACOSX_BUNDLE
${FLOYD_SOURCES}
${FLOYD_SPEAK__UT_SOURCES}
${FLOYD_SPEAK__UT_PUBLIC_HEADERS}
${FLOYD_SPEAK__UT_PROJECT_HEADERS}
${FLOYD_SPEAK__UT_PRIVATE_HEADERS}
${FLOYD_SPEAK__UT_RESOURCES}
)

target_compile_definitions(floydut PRIVATE DEBUG=1)

target_include_directories( floydut
   PUBLIC
)

target_link_libraries( floydut
${FLOYD_SPEAK__UT_STATIC_DEPENDENCIES}
${FLOYD_SPEAK__UT_DEPENDENCIES} pthread LLVM-8 readline
)
set_property(TARGET floyd PROPERTY CXX_STANDARD 17)

set_property(TARGET floydut PROPERTY CXX_STANDARD 17)


##
## benchmark
##

# Does not currently compile
#add_executable( benchmark MACOSX_BUNDLE
#${BENCHMARK_SOURCES}
#)
