#include "../include/dataFeed.hpp"

Data::Data(){
    data = parser.parse();
    currBar = data.begin();
}

Data::Data(std::string tick) : ticker(tick){
    data = parser.parse(tick);
    currBar = data.begin();
}

void Data::nextBar(){
    ++currBar;
}

Bar& Data::getBar(){
    return currBar->second;
}
bool Data::hasMoreData(){
    return currBar != data.end();
}
void Data::reset(){
    currBar = data.begin();
}