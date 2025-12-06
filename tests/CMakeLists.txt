# tests/CMakeLists.txt

# Glob test sources
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

# Test executable (uses Catch2WithMain)
add_executable(fairlanes_tests ${TEST_SOURCES})

target_compile_options(fairlanes_tests PRIVATE -O0 -g ${BIGOBJ_FLAG})

# If you’re globally using LTO, explicitly disable it for tests:
set_property(TARGET fairlanes_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)

target_precompile_headers(fairlanes_tests PRIVATE
  <catch2/catch_test_macros.hpp>
  <catch2/matchers/catch_matchers_floating_point.hpp>
)

target_link_libraries(fairlanes_tests PRIVATE fl)

# Link test binary with main code + test framework
target_link_libraries(fairlanes_tests
  PRIVATE
    Catch2::Catch2WithMain
    ExprTk::exprtk
    eventpp::eventpp
    nlohmann_json::nlohmann_json
  PUBLIC
  TracyClient
)

target_include_directories(fairlanes_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)


# Test discovery integration
include(CTest)
include(Catch)
catch_discover_tests(fairlanes_tests)
