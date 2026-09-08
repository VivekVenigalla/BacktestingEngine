add_executable(unit_tests
    test_account.cpp
    test_broker.cpp
    test_bollband.cpp
)
target_link_libraries(unit_tests PRIVATE backtest_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(unit_tests)
