#include <iostream>
#include <string>
#include "../include/csvParser.hpp"


int main(){
    Parser test;
    std::map<std::string, Bar> newData = test.parse();
    std::cout<<newData["2015-01-13 00:00:00-05:00"].open<<" test";
}