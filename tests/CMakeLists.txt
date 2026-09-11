add_executable(unit_tests
    test_account.cpp
    test_broker.cpp
    test_bollband.cpp
    test_csvparser.cpp
    test_performance.cpp
    test_smacross.cpp
    test_donchannel.cpp
    test_datafeed.cpp
    test_createstrat.cpp
    test_logger.cpp
    test_strategy.cpp
    test_pathutils.cpp
)
target_link_libraries(unit_tests PRIVATE backtest_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(unit_tests)
