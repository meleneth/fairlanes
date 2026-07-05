cmake_minimum_required(VERSION 3.20)
project(fairlanes VERSION 0.1.0 LANGUAGES CXX)

option(fairlanes_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(fairlanes_ENABLE_LTO "Enable link-time optimization" OFF)
option(BUILD_SHARED_LIBS "Build shared libs (if any)" OFF)
option(FL_BUILD_TESTS "Build tests" ON)
option(FL_ENABLE_ASSERTS "Enable runtime asserts" ON)
option(ENABLE_COVERAGE "Build with coverage instrumentation" OFF)

if(EMSCRIPTEN)
  set(FL_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
  set(FAIRLANES_ENABLE_TRACY_DEFAULT OFF)
  set(FAIRLANES_TRACY_SYSTEM_TRACING_DEFAULT OFF)
else()
  set(FAIRLANES_ENABLE_TRACY_DEFAULT ON)
  set(FAIRLANES_TRACY_SYSTEM_TRACING_DEFAULT ON)
endif()

option(FAIRLANES_ENABLE_TRACY "Enable Tracy profiling" ${FAIRLANES_ENABLE_TRACY_DEFAULT})
option(FAIRLANES_TRACY_SYSTEM_TRACING "Enable Tracy system tracing" ${FAIRLANES_TRACY_SYSTEM_TRACING_DEFAULT})

if(EMSCRIPTEN)
  set(FAIRLANES_ENABLE_TRACY OFF CACHE BOOL "Enable Tracy profiling" FORCE)
  set(FAIRLANES_TRACY_SYSTEM_TRACING OFF CACHE BOOL "Enable Tracy system tracing" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/utf-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

if(NOT CPM_SOURCE_CACHE)
  set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm-cache")
endif()

set(BIGOBJ_FLAG "")
if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(BIGOBJ_FLAG "-Wa,-mbig-obj")
elseif(MSVC)
  set(BIGOBJ_FLAG "/bigobj")
endif()

function(fairlanes_set_warnings tgt)
  if(MSVC)
    target_compile_options(${tgt} PRIVATE /W4 $<$<BOOL:${fairlanes_WARNINGS_AS_ERRORS}>:/WX>)
  else()
    target_compile_options(${tgt} PRIVATE
      -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wdouble-promotion
      $<$<BOOL:${fairlanes_WARNINGS_AS_ERRORS}>:-Werror>
    )
  endif()
endfunction()

if(fairlanes_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_msg)
  if(_ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "IPO/LTO not supported: ${_ipo_msg}")
  endif()
endif()

include(cmake/CPM.cmake)
include(FetchContent)

if(FL_BUILD_TESTS)
  CPMAddPackage("gh:catchorg/Catch2@3.5.2")
  list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
endif()

FetchContent_Declare(
  sml_src
  GIT_REPOSITORY https://github.com/boost-ext/sml.git
  GIT_TAG v1.1.8
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(sml_src)

if(TARGET sml)
  add_library(boost_sml_system_iface INTERFACE)
  target_link_libraries(boost_sml_system_iface INTERFACE sml)
  get_target_property(_sml_inc sml INTERFACE_INCLUDE_DIRECTORIES)
  if(_sml_inc)
    target_include_directories(boost_sml_system_iface SYSTEM INTERFACE ${_sml_inc})
  endif()
  if(NOT TARGET Boost::sml)
    add_library(Boost::sml ALIAS sml)
  endif()
  add_library(Boost::sml::system ALIAS boost_sml_system_iface)
else()
  add_library(boost_sml_iface INTERFACE)
  target_include_directories(boost_sml_iface SYSTEM INTERFACE ${sml_src_SOURCE_DIR}/include)
  add_library(Boost::sml ALIAS boost_sml_iface)
  add_library(Boost::sml::system ALIAS boost_sml_iface)
endif()

FetchContent_Declare(
  entt_src
  GIT_REPOSITORY https://github.com/skypjack/entt
  GIT_TAG v3.15.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(entt_src)

if(TARGET EnTT::EnTT)
  add_library(entt_system_iface INTERFACE)
  target_link_libraries(entt_system_iface INTERFACE EnTT::EnTT)
  get_target_property(_entt_inc EnTT::EnTT INTERFACE_INCLUDE_DIRECTORIES)
  if(_entt_inc)
    target_include_directories(entt_system_iface SYSTEM INTERFACE ${_entt_inc})
  endif()
  add_library(EnTT::EnTT::system ALIAS entt_system_iface)
else()
  add_library(entt_iface INTERFACE)
  target_include_directories(entt_iface SYSTEM INTERFACE ${entt_src_SOURCE_DIR}/src)
  add_library(EnTT::EnTT ALIAS entt_iface)
  add_library(EnTT::EnTT::system ALIAS entt_iface)
endif()

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui.git
  GIT_TAG v5.0.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ftxui)

CPMAddPackage(
  NAME exprtk
  GITHUB_REPOSITORY ArashPartow/exprtk
  GIT_TAG cc1b800c2bd1ac3ac260478c915d2aec6f4eb41c
  DOWNLOAD_ONLY YES
)

if(exprtk_ADDED)
  add_library(exprtk INTERFACE)
  target_include_directories(exprtk INTERFACE "${exprtk_SOURCE_DIR}")
  add_library(ExprTk::exprtk ALIAS exprtk)
endif()

CPMAddPackage(
  NAME eventpp
  GITHUB_REPOSITORY wqking/eventpp
  GIT_TAG v0.1.3
  DOWNLOAD_ONLY YES
)

if(eventpp_ADDED)
  add_library(eventpp INTERFACE)
  target_include_directories(eventpp INTERFACE "${eventpp_SOURCE_DIR}/include")
  add_library(eventpp::eventpp ALIAS eventpp)
endif()

CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  GIT_TAG v3.11.3
)

CPMAddPackage(
  NAME pcg-cpp
  GITHUB_REPOSITORY imneme/pcg-cpp
  GIT_TAG master
)

if(pcg-cpp_ADDED)
  add_library(pcg-cpp INTERFACE)
  target_include_directories(pcg-cpp INTERFACE
    ${pcg-cpp_SOURCE_DIR}/include
    ${pcg-cpp_SOURCE_DIR}
  )
endif()

CPMAddPackage(
  NAME CLI11
  GITHUB_REPOSITORY CLIUtils/CLI11
  VERSION 2.4.2
  DOWNLOAD_ONLY YES
)

if(CLI11_ADDED)
  add_library(CLI11 INTERFACE)
  target_include_directories(CLI11
    INTERFACE
      $<BUILD_INTERFACE:${CLI11_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
endif()

CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 12.1.0
  OPTIONS
    "FMT_DOC OFF"
    "FMT_TEST OFF"
)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)

CPMAddPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  VERSION 1.17.0
  OPTIONS
    "SPDLOG_BUILD_SHARED OFF"
    "SPDLOG_BUILD_EXAMPLE OFF"
    "SPDLOG_BUILD_TESTS OFF"
)

add_library(fairlanes_tracy INTERFACE)

if(FAIRLANES_ENABLE_TRACY)
  set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy profiling" FORCE)
  set(TRACY_ON_DEMAND ON CACHE BOOL "Enable Tracy on-demand connection" FORCE)
  set(TRACY_NO_EXIT OFF CACHE BOOL "Tracy persists after disconnect" FORCE)

  FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.13.1
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
  )

  FetchContent_MakeAvailable(tracy)

  target_link_libraries(fairlanes_tracy INTERFACE TracyClient)
  target_compile_definitions(fairlanes_tracy INTERFACE TRACY_ENABLE)

  if(NOT FAIRLANES_TRACY_SYSTEM_TRACING)
    target_compile_definitions(TracyClient PRIVATE TRACY_NO_SYSTEM_TRACING)
  endif()
endif()

add_library(Fairlanes::Tracy ALIAS fairlanes_tracy)

include(CTest)

if(FL_BUILD_TESTS)
  enable_testing()
endif()

if(ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Building with coverage instrumentation")
    add_compile_options(
      -O0
      -g
      --coverage
      -fno-inline
      -fno-omit-frame-pointer
    )
    add_link_options(--coverage)
  endif()
endif()

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
file(GLOB_RECURSE FAIRLANES_FORMAT_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.hpp
  ${CMAKE_SOURCE_DIR}/tests/*.cpp
  ${CMAKE_SOURCE_DIR}/tests/*.hpp
)

if(CLANG_FORMAT_EXECUTABLE)
  add_custom_target(clang-format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${FAIRLANES_FORMAT_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting Fairlanes C++ sources"
  )
else()
  add_custom_target(clang-format
    COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found"
    COMMAND ${CMAKE_COMMAND} -E false
  )
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(IWYU_EXECUTABLE NAMES include-what-you-use iwyu)
set(FAIRLANES_IWYU_ONLY_REGEX "" CACHE STRING "Only run IWYU for source paths matching this regex")
set(FAIRLANES_IWYU_EXCLUDE_REGEX "src/fl/skills/thump\\.cpp" CACHE STRING "Skip IWYU for source paths matching this regex")

if(IWYU_EXECUTABLE)
  set(FAIRLANES_IWYU_ARGS
      --compile-commands ${CMAKE_BINARY_DIR}/compile_commands.json
      --iwyu ${IWYU_EXECUTABLE}
      --root ${CMAKE_SOURCE_DIR}
      --timeout 90
  )
  if(FAIRLANES_IWYU_ONLY_REGEX)
    list(APPEND FAIRLANES_IWYU_ARGS --only-regex ${FAIRLANES_IWYU_ONLY_REGEX})
  endif()
  if(FAIRLANES_IWYU_EXCLUDE_REGEX)
    list(APPEND FAIRLANES_IWYU_ARGS --exclude-regex ${FAIRLANES_IWYU_EXCLUDE_REGEX})
  endif()

  add_custom_target(include-what-you-use
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake/run_iwyu.py
            ${FAIRLANES_IWYU_ARGS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running include-what-you-use over Fairlanes sources"
  )
else()
  add_custom_target(include-what-you-use
    COMMAND ${CMAKE_COMMAND} -E echo "include-what-you-use not found"
    COMMAND ${CMAKE_COMMAND} -E false
  )
endif()

find_program(GCOVR_EXECUTABLE gcovr)

find_program(RUBY_EXECUTABLE ruby)
if(NOT RUBY_EXECUTABLE)
  message(FATAL_ERROR "ruby not found; required for Fairlanes content generation")
endif()

file(GLOB_RECURSE FAIRLANES_CONTENT_RUBY_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/ruby/fairlanes_content/lib/*.rb"
)

set(FAIRLANES_CONTENT_GENERATED_DIR
  "${CMAKE_BINARY_DIR}/generated/fairlanes_content")
set(FAIRLANES_CONTENT_GENERATED_TEST
  "${FAIRLANES_CONTENT_GENERATED_DIR}/generated_decal_content_tests.cpp")
set(FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_HPP
  "${FAIRLANES_CONTENT_GENERATED_DIR}/fl/generated/skill_content.hpp")
set(FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_CPP
  "${FAIRLANES_CONTENT_GENERATED_DIR}/fl/generated/skill_content.cpp")
set(FAIRLANES_CONTENT_GENERATED_REPORT
  "${FAIRLANES_CONTENT_GENERATED_DIR}/decal_content_balance.md")
set(FAIRLANES_CONTENT_GENERATED_GALLERY
  "${FAIRLANES_CONTENT_GENERATED_DIR}/effect_gallery.md")
set(FAIRLANES_CONTENT_GENERATED_MANIFEST
  "${FAIRLANES_CONTENT_GENERATED_DIR}/content_manifest.json")
set(FAIRLANES_CONTENT_GENERATED_MANIFEST_SCHEMA
  "${FAIRLANES_CONTENT_GENERATED_DIR}/content_manifest.schema.json")

add_custom_command(
  OUTPUT
    "${FAIRLANES_CONTENT_GENERATED_TEST}"
    "${FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_HPP}"
    "${FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_CPP}"
    "${FAIRLANES_CONTENT_GENERATED_REPORT}"
    "${FAIRLANES_CONTENT_GENERATED_GALLERY}"
    "${FAIRLANES_CONTENT_GENERATED_MANIFEST}"
    "${FAIRLANES_CONTENT_GENERATED_MANIFEST_SCHEMA}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${FAIRLANES_CONTENT_GENERATED_DIR}"
  COMMAND "${RUBY_EXECUTABLE}"
          "${CMAKE_SOURCE_DIR}/scripts/fairlanes_content_codegen.rb"
          --out-dir "${FAIRLANES_CONTENT_GENERATED_DIR}"
  DEPENDS
    "${CMAKE_SOURCE_DIR}/scripts/fairlanes_content_codegen.rb"
    "${CMAKE_SOURCE_DIR}/scripts/fairlanes_content_declarations.rb"
    ${FAIRLANES_CONTENT_RUBY_SOURCES}
  COMMENT "Generating Fairlanes content artifacts"
  VERBATIM
)

add_custom_target(fairlanes_content_check
  DEPENDS
    "${FAIRLANES_CONTENT_GENERATED_TEST}"
    "${FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_HPP}"
    "${FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_CPP}"
    "${FAIRLANES_CONTENT_GENERATED_REPORT}"
    "${FAIRLANES_CONTENT_GENERATED_GALLERY}"
    "${FAIRLANES_CONTENT_GENERATED_MANIFEST}"
    "${FAIRLANES_CONTENT_GENERATED_MANIFEST_SCHEMA}"
)

if(GCOVR_EXECUTABLE)
  add_custom_target(coverage-lcov
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${GCOVR_EXECUTABLE}
      -r ${CMAKE_SOURCE_DIR}
      --exclude-directories ${CMAKE_BINARY_DIR}
      --lcov
      -o ${CMAKE_BINARY_DIR}/lcov.info
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating lcov.info with gcovr"
  )
else()
  message(STATUS "gcovr not found; coverage-lcov target disabled")
endif()

if(ENABLE_COVERAGE)
  find_program(PERL_EXECUTABLE perl)
  find_program(LCOV_SCRIPT lcov)
  find_program(GENHTML_SCRIPT genhtml)

  if(NOT PERL_EXECUTABLE)
    message(FATAL_ERROR "perl not found (required for lcov)")
  endif()

  if(NOT LCOV_SCRIPT OR NOT GENHTML_SCRIPT)
    message(FATAL_ERROR "lcov/genhtml not found")
  endif()

  add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${PERL_EXECUTABLE} ${LCOV_SCRIPT}
            --capture
            --directory .
            --output-file coverage.info
    COMMAND ${PERL_EXECUTABLE} ${LCOV_SCRIPT}
            --remove coverage.info
            "*/_deps/*"
            "*/tests/*"
            "/usr/*"
            "*build-coverage*"
            "*15.2.0*"
            --output-file coverage.cleaned.info
    COMMAND ${PERL_EXECUTABLE} ${GENHTML_SCRIPT}
            coverage.cleaned.info
            --output-directory coverage/html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

file(GLOB_RECURSE FAIRLANES_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/fl/*.cpp
)

add_library(fl STATIC ${FAIRLANES_SOURCES})
target_sources(fl PRIVATE "${FAIRLANES_CONTENT_GENERATED_SKILL_CONTENT_CPP}")
add_dependencies(fl fairlanes_content_check)

target_include_directories(fl
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${FAIRLANES_CONTENT_GENERATED_DIR}
  PRIVATE
    ${entt_SOURCE_DIR}/single_include
)

target_compile_definitions(fl
  PUBLIC
    FL_ENABLE_ASSERTS=$<BOOL:${FL_ENABLE_ASSERTS}>
)

target_link_libraries(fl
  PUBLIC
    pcg-cpp
    EnTT::EnTT
    Boost::sml::system
    ftxui::screen
    ftxui::dom
    ftxui::component
    ExprTk::exprtk
    spdlog::spdlog
    eventpp::eventpp
    CLI11
    nlohmann_json::nlohmann_json
    Fairlanes::Tracy
)

target_compile_options(fl PRIVATE
  -Wall -Wextra -Wpedantic -Werror
  -O0 -g ${BIGOBJ_FLAG}
)

if(EMSCRIPTEN)
  target_compile_options(fl PRIVATE -pthread)
endif()

file(GLOB_RECURSE SEERIN_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/sr/*.cpp
)

add_library(sr STATIC ${SEERIN_SOURCES})

target_include_directories(sr
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
  PRIVATE
    ${entt_SOURCE_DIR}/single_include
)

target_link_libraries(sr
  PUBLIC
    pcg-cpp
    EnTT::EnTT
    Boost::sml::system
    ftxui::screen
    ftxui::dom
    ftxui::component
    ExprTk::exprtk
    spdlog::spdlog
    eventpp::eventpp
    CLI11
    nlohmann_json::nlohmann_json
    Fairlanes::Tracy
)

target_compile_options(sr PRIVATE
  -Wall -Wextra -Wpedantic -Werror
  -O0 -g ${BIGOBJ_FLAG}
)

if(EMSCRIPTEN)
  target_compile_options(sr PRIVATE -pthread)
endif()

add_executable(fairlanes ${CMAKE_SOURCE_DIR}/src/fairlanes.cpp)

target_include_directories(fairlanes
  PRIVATE
    ${entt_SOURCE_DIR}/single_include
)
target_include_directories(fairlanes
    SYSTEM PRIVATE
    ${eventpp_SOURCE_DIR}/include
)

target_compile_definitions(fairlanes
  PUBLIC
    FL_ENABLE_ASSERTS=$<BOOL:${FL_ENABLE_ASSERTS}>
)

target_link_libraries(fairlanes
  PRIVATE
    fl
    sr
    pcg-cpp
    ftxui::screen
    ftxui::dom
    ftxui::component
    Boost::sml::system
    EnTT::EnTT::system
    ExprTk::exprtk
    eventpp::eventpp
    CLI11
    spdlog::spdlog
    Fairlanes::Tracy
)

if(EMSCRIPTEN)
  target_compile_definitions(fairlanes PRIVATE FMT_HEADER_ONLY)
  target_link_libraries(fairlanes PRIVATE fmt::fmt-header-only)

  target_compile_options(fairlanes PRIVATE
    -pthread
  )

  target_link_options(fairlanes PRIVATE
    -pthread
    -sUSE_PTHREADS=1
    -sPTHREAD_POOL_SIZE=4
    -sALLOW_MEMORY_GROWTH=1
    -sASSERTIONS=1
    -sSTACK_OVERFLOW_CHECK=2
    -sPROXY_TO_PTHREAD
    -sASYNCIFY
  )

  set_target_properties(fairlanes PROPERTIES OUTPUT_NAME "fairlanes")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
  target_link_libraries(fairlanes PRIVATE fmt::fmt)
endif()

fairlanes_set_warnings(fairlanes)
target_compile_features(fairlanes PRIVATE cxx_std_20)

if(WIN32)
  target_compile_definitions(fairlanes PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(FL_BUILD_TESTS)
  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/tests/*.cpp
  )
  list(APPEND TEST_SOURCES "${FAIRLANES_CONTENT_GENERATED_TEST}")

  add_executable(fairlanes_tests ${TEST_SOURCES})
  add_dependencies(fairlanes_tests fairlanes_content_check)

  target_compile_options(fairlanes_tests PRIVATE -O0 -g ${BIGOBJ_FLAG})
  set_property(TARGET fairlanes_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)

  target_precompile_headers(fairlanes_tests PRIVATE
    <catch2/catch_test_macros.hpp>
    <catch2/matchers/catch_matchers_floating_point.hpp>
  )

  target_link_libraries(fairlanes_tests
    PRIVATE
      fl
      sr
      Catch2::Catch2WithMain
      ExprTk::exprtk
      eventpp::eventpp
      nlohmann_json::nlohmann_json
      Fairlanes::Tracy
  )

  target_compile_definitions(fairlanes_tests
    PUBLIC
      FL_ENABLE_ASSERTS=$<BOOL:${FL_ENABLE_ASSERTS}>
  )

  target_include_directories(fairlanes_tests
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/tests/support
  )

  include(Catch)
  catch_discover_tests(fairlanes_tests)

  if(ENABLE_COVERAGE)
    add_dependencies(coverage fairlanes_tests)
  endif()
endif()
