#include "../include/dataFeed.hpp"

Data::Data(){
    data = parser.parse();
    //data.begin() gives an iterator pointing at the first(earliest, since
    //std::map keeps keys sorted) bar - this is what currBar starts at
    currBar = data.begin();
}

Data::Data(std::string id, std::string tick, std::string path) : ticker(tick), ID(id), PATH(path){
    data = parser.parse(id, path);
    currBar = data.begin();
}

void Data::nextBar(){
    //++currBar moves the iterator forward to the next entry in the map -
    //since std::map is sorted by key(the date string), this steps forward
    //one day at a time
    ++currBar;
}

Bar& Data::getBar(){
    //currBar->second is the Bar half of the current(key, Bar) pair the
    //iterator points at - ->first would give you the date string instead
    return currBar->second;
}
bool Data::hasMoreData(){
    //data.end() is a special "one past the last real element" iterator -
    //comparing against it is the standard way to check "have we walked off
    //the end of the container yet"
    return currBar != data.end();
}
void Data::reset(){
    currBar = data.begin();
}

std::map<std::string, Bar> Data::sliceBars(const std::string& startDate, const std::string& endDate) const{
    std::map<std::string, Bar> slice;

    //data's keys are YYYY-MM-DD date strings, which sort lexicographically
    //in the exact same order they sort chronologically - so std::map's own
    //key ordering(the same ordering lower_bound/upper_bound search over)
    //already IS date order, no separate date-parsing needed
    //
    //lower_bound(startDate): the first entry with a key >= startDate
    //upper_bound(endDate): the first entry with a key > endDate
    //[lower_bound(start), upper_bound(end)) is therefore every bar with
    //startDate <= date <= endDate - inclusive on both ends, matching
    //WalkForwardWindow's own inSampleStart/inSampleEnd convention
    auto rangeBegin = data.lower_bound(startDate);
    auto rangeEnd = data.upper_bound(endDate);

    //std::map's insert(first, last) copies every (key, Bar) pair in that
    //iterator range straight into slice, in one call, rather than looping
    //and inserting one at a time by hand
    slice.insert(rangeBegin, rangeEnd);

    return slice;
}
