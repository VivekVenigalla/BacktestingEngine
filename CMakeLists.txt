cmake_minimum_required( VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project( my_project VERSION 1.0 )

find_package(nlohmann_json 3.10.5 REQUIRED)

# Bake this checkout's absolute path into a generated header (see
# include/projectPaths.hpp.in) so main.cpp/logger.cpp no longer need a
# hardcoded path that only worked on one machine.
configure_file(
    include/projectPaths.hpp.in
    ${CMAKE_BINARY_DIR}/generated/projectPaths.hpp
    @ONLY
)

# All engine logic EXCEPT main.cpp, compiled once into a static library.
# Both the real program (runny) and the test program (unit_tests) link
# against this, each supplying their own main().
add_library(backtest_core STATIC
    src/csvParser.cpp
    src/account.cpp
    src/broker.cpp
    src/structures.cpp
    src/dataFeed.cpp
    src/strategy.cpp
    src/createStrat.cpp
    src/strategies/smaCross.cpp
    src/strategies/bollBand.cpp
    src/strategies/donChannel.cpp
    src/performanceEval.cpp
    src/logger.cpp
    src/simulationRunner.cpp
)
target_include_directories(backtest_core PUBLIC include ${CMAKE_BINARY_DIR}/generated)
# PUBLIC (not PRIVATE): several public headers (createStrat.hpp,
# simulationRunner.hpp) #include nlohmann/json.hpp directly, so anything
# linking this library needs those include paths propagated to it too.
target_link_libraries(backtest_core PUBLIC nlohmann_json::nlohmann_json)

# The real program: just the entry point, linked against the library above.
add_executable(runny src/main.cpp)
target_link_libraries(runny PRIVATE backtest_core)

# --- Testing ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

enable_testing()
add_subdirectory(tests)
