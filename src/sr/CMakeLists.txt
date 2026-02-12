# Declare the fairlanes library

file(GLOB_RECURSE SEERIN_SOURCES CONFIGURE_DEPENDS *.cpp)

# Declare the library
add_library(sr STATIC
  ${SEERIN_SOURCES}
)

# Set include path for other targets
target_include_directories(sr
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

# Link FTXUI (required by battle_screen.cpp)
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
  TracyClient
)

# Optional: compile flags
target_compile_options(sr PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)

target_include_directories(sr PRIVATE
  ${entt_SOURCE_DIR}/single_include
)

target_compile_options(sr PRIVATE -O0 -g ${BIGOBJ_FLAG})

