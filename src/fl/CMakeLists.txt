# Declare the fairlanes library

file(GLOB_RECURSE FAIRLANES_SOURCES CONFIGURE_DEPENDS *.cpp)

# Declare the library
add_library(fl STATIC
  ${FAIRLANES_SOURCES}
)

# Set include path for other targets
target_include_directories(fl
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(fl
  PUBLIC
    FL_ENABLE_ASSERTS=$<BOOL:${FL_ENABLE_ASSERTS}>
)

# Link FTXUI (required by battle_screen.cpp)
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
  TracyClient
)

# Optional: compile flags
target_compile_options(fl PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

target_include_directories(fl PRIVATE
  ${entt_SOURCE_DIR}/single_include
)

target_compile_options(fl PRIVATE -O0 -g ${BIGOBJ_FLAG})

