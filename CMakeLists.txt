cmake_minimum_required( VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project( my_project VERSION 1.0 )

find_package(nlohmann_json 3.10.5 REQUIRED)

add_executable( runny src/main.cpp src/csvParser.cpp src/account.cpp  src/broker.cpp src/structures.cpp src/dataFeed.cpp src/strategy.cpp src/createStrat.cpp src/strategies/smaCross.cpp src/strategies/bollBand.cpp src/strategies/donChannel.cpp src/performanceEval.cpp src/logger.cpp)
target_link_libraries(runny PRIVATE nlohmann_json::nlohmann_json)
target_include_directories( runny PUBLIC include )