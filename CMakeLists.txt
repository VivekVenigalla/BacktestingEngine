cmake_minimum_required( VERSION 3.15)

project( my_project VERSION 1.0 )

add_executable( runny src/main.cpp src/csvParser.cpp src/account.cpp  src/broker.cpp src/structures.cpp src/dataFeed.cpp src/strategy.cpp src/createStrat.cpp src/strategies/smaCross.cpp src/strategies/bollBand.cpp src/strategies/donChannel.cpp src/performanceEval.cpp src/logger.cpp)
target_include_directories( runny PUBLIC include )