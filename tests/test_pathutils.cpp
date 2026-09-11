#include <catch2/catch_test_macros.hpp>
#include "pathUtils.hpp"
#include "projectPaths.hpp"

//projectPaths.hpp gives PROJECT_SOURCE_DIR - the same build-time-generated
//constant the real program passes to resolveFeedPath, so these tests
//exercise exactly the inputs main.cpp actually uses

TEST_CASE("resolveFeedPath resolves a real config-style relative path against projectRoot/src", "[pathutils]") {
    //this is the exact string every batchConfig json on disk stores for
    //csv_filepath - see config/batchConfig/test_batch.json
    std::string resolved = resolveFeedPath("../data/TSLA_1D_2024-01-01_2024-12-31.csv", PROJECT_SOURCE_DIR);
    fs::path expected = fs::path(PROJECT_SOURCE_DIR) / "data" / "TSLA_1D_2024-01-01_2024-12-31.csv";

    REQUIRE(resolved == expected.lexically_normal().string());
}

TEST_CASE("resolveFeedPath passes an already-absolute path through unchanged", "[pathutils]") {
    fs::path absolutePath = fs::path(PROJECT_SOURCE_DIR) / "data" / "AAPL_1d.csv";
    std::string resolved = resolveFeedPath(absolutePath.string(), PROJECT_SOURCE_DIR);

    REQUIRE(resolved == absolutePath.string());
}

TEST_CASE("resolveFeedPath normalizes a path that climbs past the project root", "[pathutils]") {
    //legal-but-unusual: nothing stops a config from writing a path with
    //more ".."s than it needs. lexically_normal() just collapses them
    //arithmetically - it doesn't check the result actually exists on disk
    std::string resolved = resolveFeedPath("../../outside_project/file.csv", PROJECT_SOURCE_DIR);
    fs::path expected = (fs::path(PROJECT_SOURCE_DIR) / "src" / "../../outside_project/file.csv").lexically_normal();

    REQUIRE(resolved == expected.string());
}
