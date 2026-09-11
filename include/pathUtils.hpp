#pragma once
#include <string>
#include <filesystem>

//see logger.hpp for why this alias exists - lets the rest of this file
//write the shorter "fs::path" instead of spelling out "std::filesystem::path"
namespace fs = std::filesystem;

//every batch config on disk stores csv_filepath as something like
//"../data/TSLA_....csv" - a path that's only correct if the program's
//current working directory happens to be src/, which csv_download.py and
//GUI.py both assume but running ./runny from build/ (or anywhere else)
//does not. resolveFeedPath fixes that: it turns a config's raw path into
//one that works no matter where the program was launched from, by
//resolving it against the project's own on-disk location instead of
//whatever directory the shell happened to be in
std::string resolveFeedPath(const std::string& rawPath, const std::string& projectRoot);
