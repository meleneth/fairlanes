# Make narrow string literals compiled as UTF-8 everywhere.
if(MSVC)
  add_compile_options(/utf-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

add_subdirectory(fl)
add_subdirectory(sr)

add_executable(fairlanes fairlanes.cpp)

target_link_libraries(fairlanes PRIVATE fl sr)

target_include_directories(fairlanes PRIVATE
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


if(EMSCRIPTEN)
  target_compile_definitions(fairlanes PRIVATE FMT_HEADER_ONLY)
  target_link_libraries(fairlanes PRIVATE fmt::fmt-header-only)
  target_compile_options(fairlanes PRIVATE -sUSE_PTHREADS=1)
  target_link_options(fairlanes PRIVATE -pthread -sPROXY_TO_PTHREAD -sASYNCIFY)
else()
  target_link_libraries(fairlanes PRIVATE fmt::fmt)
endif()



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
)

if(EMSCRIPTEN)
  target_compile_options(fairlanes PRIVATE -O3 -flto)
  target_link_options(fairlanes PRIVATE -O3 -flto -sASSERTIONS=0)  target_compile_options(fairlanes PRIVATE -O3 -flto)
  target_link_options(fairlanes PRIVATE -O3 -flto -sASSERTIONS=0)
else()
  target_link_libraries(fairlanes PRIVATE TracyClient)
  target_compile_definitions(fairlanes PRIVATE TRACY_ENABLE)
endif()

fairlanes_set_warnings(fairlanes)
target_compile_features(fairlanes PRIVATE cxx_std_20)

if(WIN32)
  target_compile_definitions(fairlanes PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
