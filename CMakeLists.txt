cmake_minimum_required(VERSION 3.20)
project(fairlanes VERSION 0.1.0 LANGUAGES CXX)


option(fairlanes_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(fairlanes_ENABLE_LTO          "Enable link-time optimization" OFF)
option(BUILD_SHARED_LIBS            "Build shared libs (if any)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CPM_SOURCE_CACHE)
  set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm-cache")
endif()


# Big object flag, only where it makes sense
set(BIGOBJ_FLAG "")

# MinGW / GCC on Windows
if (MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(BIGOBJ_FLAG "-Wa,-mbig-obj")
elseif (MSVC)
  # MSVC equivalent if you ever need it
  set(BIGOBJ_FLAG "/bigobj")
endif()

include(cmake/CPM.cmake)

CPMAddPackage("gh:catchorg/Catch2@3.5.2")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

function(fairlanes_set_warnings tgt)
  if(MSVC)
    target_compile_options(${tgt} PRIVATE /W4 $<$<BOOL:${fairlanes_WARNINGS_AS_ERRORS}>:/WX>)
  else()
    target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wdouble-promotion
      $<$<BOOL:${fairlanes_WARNINGS_AS_ERRORS}>:-Werror>)
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

include(FetchContent)
# --- boost-ext/sml -----------------------------------------------------------
FetchContent_Declare(
  sml_src
  GIT_REPOSITORY https://github.com/boost-ext/sml.git
  GIT_TAG        v1.1.8
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(sml_src)

# Prefer upstream target if present; else define our own
if(TARGET sml)
  # Create a SYSTEM-wrapped shim, then alias with '::'
  add_library(boost_sml_system_iface INTERFACE)
  target_link_libraries(boost_sml_system_iface INTERFACE sml)
  # Re-expose sml's include dirs as SYSTEM to mute 3rd-party warnings
  get_target_property(_sml_inc sml INTERFACE_INCLUDE_DIRECTORIES)
  if(_sml_inc)
    target_include_directories(boost_sml_system_iface SYSTEM INTERFACE ${_sml_inc})
  endif()
  # Friendly stable aliases
  if(NOT TARGET Boost::sml)
    add_library(Boost::sml ALIAS sml)
  endif()
  add_library(Boost::sml::system ALIAS boost_sml_system_iface)
else()
  # Fallback: build our own header-only target
  add_library(boost_sml_iface INTERFACE)
  target_include_directories(boost_sml_iface SYSTEM INTERFACE ${sml_src_SOURCE_DIR}/include)
  add_library(Boost::sml ALIAS boost_sml_iface)
  add_library(Boost::sml::system ALIAS boost_sml_iface)
endif()

# --- EnTT --------------------------------------------------------------------
FetchContent_Declare(
  entt_src
  GIT_REPOSITORY https://github.com/skypjack/entt
  GIT_TAG        v3.15.0
  GIT_SHALLOW    TRUE
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


# --- FTXUI -------------------------------------------------------------------
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui.git
  GIT_TAG        v5.0.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(ftxui)
# targets: ftxui::screen ftxui::dom ftxui::component

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

# --- ExprTk via CPM ----------------------------------------------------------
# Pin to a specific commit for reproducibility (Jan 1, 2024)
# See: https://github.com/ArashPartow/exprtk/commits/master
CPMAddPackage(
  NAME exprtk
  GITHUB_REPOSITORY ArashPartow/exprtk
  GIT_TAG cc1b800c2bd1ac3ac260478c915d2aec6f4eb41c   # replace with the full SHA if you prefer
  DOWNLOAD_ONLY YES
)

# ExprTk is just a single header (exprtk.hpp). Create an INTERFACE target.
if(exprtk_ADDED)
  add_library(exprtk INTERFACE)
  target_include_directories(exprtk INTERFACE "${exprtk_SOURCE_DIR}")
  add_library(ExprTk::exprtk ALIAS exprtk)
endif()

# eventpp via CPM
CPMAddPackage(
  NAME eventpp
  GITHUB_REPOSITORY wqking/eventpp
  GIT_TAG v0.1.3          # latest release as of now
  DOWNLOAD_ONLY YES       # we'll make our own interface target
)

if(eventpp_ADDED)
  add_library(eventpp INTERFACE)
  target_include_directories(eventpp INTERFACE
    "${eventpp_SOURCE_DIR}/include"
  )

  # Optional: provide the "official" namespace alias
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

# pcg-cpp is header-only, so we make a tiny INTERFACE target for it
if(pcg-cpp_ADDED)
  add_library(pcg-cpp INTERFACE)
  target_include_directories(pcg-cpp INTERFACE
    ${pcg-cpp_SOURCE_DIR}/include       # contains pcg_extras/
    ${pcg-cpp_SOURCE_DIR}               # fallback
  )
endif()


# CLI11 as pure headers via CPM — no add_subdirectory, no policy spam
CPMAddPackage(
  NAME CLI11
  GITHUB_REPOSITORY CLIUtils/CLI11
  VERSION 2.4.2
  DOWNLOAD_ONLY YES        # <- key: fetch only, don't configure/build
)

if (CLI11_ADDED)
  add_library(CLI11 INTERFACE)
  # mark as SYSTEM to silence third-party warnings, optional
  target_include_directories(CLI11
    INTERFACE
      $<BUILD_INTERFACE:${CLI11_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )
endif()


# fmt (compiled library)
CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 10.2.1
  OPTIONS
    "FMT_DOC OFF"
    "FMT_TEST OFF"
)

# Tell spdlog to use the external fmt we just added
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)

CPMAddPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  VERSION 1.14.1
  OPTIONS
    "SPDLOG_BUILD_SHARED OFF"
    "SPDLOG_BUILD_EXAMPLE OFF"
    "SPDLOG_BUILD_TESTS OFF"
)


# Tracy profiler
set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy profiling" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "Enable Tracy on-demand connection" FORCE)
set(TRACY_NO_EXIT OFF CACHE BOOL "Tracy persists after disconnect" FORCE)

FetchContent_Declare(
tracy
GIT_REPOSITORY https://github.com/wolfpld/tracy.git
GIT_REVISION 05cceee
GIT_SHALLOW true
GIT_PROGRESS true
)

FetchContent_MakeAvailable(tracy)
if (MINGW)
  target_compile_definitions(TracyClient PRIVATE TRACY_NO_SYSTEM_TRACING)
endif()
include(CTest)       # or: enable CTest integration
enable_testing()     # actually turn on test support

# top-level CMakeLists.txt

option(ENABLE_COVERAGE "Build with coverage instrumentation" OFF)

if (ENABLE_COVERAGE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
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


find_program(GCOVR_EXECUTABLE gcovr)

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


if (ENABLE_COVERAGE)
  find_program(PERL_EXECUTABLE perl)
  find_program(LCOV_SCRIPT lcov)
  find_program(GENHTML_SCRIPT genhtml)

  if (NOT PERL_EXECUTABLE)
    message(FATAL_ERROR "perl not found (required for lcov)")
  endif()

  if (NOT LCOV_SCRIPT OR NOT GENHTML_SCRIPT)
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



# --- Your code ---------------------------------------------------------------
add_subdirectory(src)
add_subdirectory(tests)
# add_subdirectory(external)  # only if you actually have it
if (ENABLE_COVERAGE)

add_dependencies(coverage fairlanes_tests)
endif()

if (EMSCRIPTEN)
  target_compile_definitions(fairlanes PRIVATE __EMSCRIPTEN__)
  target_link_options(fairlanes PRIVATE
    "-sALLOW_MEMORY_GROWTH=1"
    "-sASSERTIONS=1"
    # "--preload-file" "assets@/assets"   # if you have assets
  )
  set_target_properties(fairlanes PROPERTIES OUTPUT_NAME "fairlanes")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()
