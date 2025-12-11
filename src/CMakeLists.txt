# Make narrow string literals compiled as UTF-8 everywhere.
if(MSVC)
  add_compile_options(/utf-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

add_subdirectory(fl)

add_executable(fairlanes fairlanes.cpp)

target_link_libraries(fairlanes PRIVATE fl)

target_include_directories(fairlanes PRIVATE
  ${entt_SOURCE_DIR}/single_include
)
target_include_directories(fairlanes
  SYSTEM PRIVATE
    ${eventpp_SOURCE_DIR}/include
)

target_link_libraries(fairlanes PRIVATE
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
  fmt::fmt
  PUBLIC
  TracyClient
)

fairlanes_set_warnings(fairlanes)
target_compile_features(fairlanes PRIVATE cxx_std_20)

if(WIN32)
  target_compile_definitions(fairlanes PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
