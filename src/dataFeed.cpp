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
