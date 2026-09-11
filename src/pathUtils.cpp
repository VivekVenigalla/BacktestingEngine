#include "pathUtils.hpp"

std::string resolveFeedPath(const std::string& rawPath, const std::string& projectRoot){
    fs::path raw(rawPath);

    //an already-absolute path (e.g. someone points a config straight at
    ///Users/.../some_file.csv) needs no resolving at all - pass it through
    //unchanged
    if(raw.is_absolute()){
        return raw.string();
    }

    //every relative csv_filepath in the repo is written relative to src/
    //(that's where csv_download.py/GUI.py both write and read them from),
    //so resolving against projectRoot/src reproduces that same convention
    //no matter what directory the program was actually launched from.
    //lexically_normal() then collapses the ".."/"." pieces (e.g.
    //"src/../data/TSLA.csv" -> "data/TSLA.csv") into a clean final path
    fs::path resolved = (fs::path(projectRoot) / "src" / raw).lexically_normal();
    return resolved.string();
}
