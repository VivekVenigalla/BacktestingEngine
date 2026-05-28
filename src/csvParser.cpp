#include "../include/csvParser.hpp"
//the include above already includes string, vector, and structures
#include <iostream>
#include <fstream>
#include <sstream>




std::vector<Bar> Parser::parse(){
    //std::vector<Bar> data;
    //access the csv file for read only
    //to read and writ use fstream
    std::ifstream file(DATA_PATH);

    //check if the file was accessed or not
    if(!file.is_open()){
        std::cout << "File failed to open";
        return data;
    }

    //obtain the first line and print it for confirmation
    std::getline(file, row);//the s reads the line as a string
    //redo this
    if(row != " "){
        std::cout << "Parser functional...Now parsing...";
    }

    while(std::getline(file, row)){
        //although we have a row, we need to process it to get every individual enty data and ensure they are properly types
        //convert the row into a stringstream
        std::stringstream ss(row);
        
        //since not all of the types are the same, I have to manually do it.
        
        std::string date_temp;
        std::string open_temp;
        std::string close_temp;
        std::string high_temp;
        std::string low_temp;
        std::string volume_temp;
        
        Bar newBar;

        //get each data point and 
        std::getline(ss, date_temp, ',');
        std::getline(ss, open_temp, ',');
        std::getline(ss, high_temp, ',' );
        std::getline(ss, low_temp, ',' );
        std::getline(ss, close_temp, ',' );
        std::getline(ss, volume_temp, ',' );

        newBar.date = date_temp;
        newBar.open = std::stod(open_temp);
        newBar.high = std::stod(high_temp);
        newBar.low = std::stod(low_temp);
        newBar.close = std::stod(close_temp);
        newBar.volume = std::stol(volume_temp);
        std::cout << newBar.date << " record uploaded...\n";

        data.push_back(newBar);
        
    }
    
    return data;
    //iterate through the csv file
}