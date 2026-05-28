#include <iostream>
#include <string>
#include "../include/csvParser.hpp"


int main(){
    Parser test;
    std::vector<Bar> newData = test.parse();
    std::cout<<newData[0].open<<" test";
}