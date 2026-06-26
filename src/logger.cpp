#include "../include/logger.hpp"
#include <fstream>


void Logger::logSnapshot(std::string date, std::unordered_map<std::string, Bar> bar, double balance, double equity, std::unordered_map<std::string, Position> pos){
    //insert the snapshot into the structure
    fullHistory.dates.push_back(date);
    fullHistory.bars.push_back(bar);
    fullHistory.balances.push_back(balance);
    fullHistory.totalEquity.push_back(equity);
    fullHistory.positions.push_back(pos);

    //create a lookup entry for fast access
    lookupMap[date] = fullHistory.dates.size()-1;
    
    
}

void Logger::printSnapshot(std::string date){
    fullHistory.print_with_date(lookupMap[date]);
}

//filename requires a proper directory
void Logger::exportCSV(std::string filename){
    //ofstream only allows writing to files
    std::ofstream file(filename);

    if(!file){
        std::cout<<"File " << filename << " unable to be created. Terminating export..." << std::endl;
        return;
    }
    else{
        //only focus on these values shown below, will look into position and bar values later
        file << "Date,Balance,Equity,Ticker,BarOpen,BarHigh,BarLow,BarClose,BarVolume,Quantity,AEP\n";
        
        //loop through the vectors and input them one by one
        //for each ticker the csv has another row with the same date
        for(int i =0; i < fullHistory.dates.size(); i++){
            for(const auto& [key, value] : fullHistory.bars[i]){
                file << fullHistory.dates[i] << ","
                << fullHistory.balances[i] << ","
                << fullHistory.totalEquity[i] << ","
                << value.open << ","
                << value.high << ","
                << value.low << ","
                << value.close << ","
                << value.volume << ","
                << fullHistory.positions[i][key].quantity << ","
                << fullHistory.positions[i][key].average_entry_price << "\n";
                 
            }
        }

        file.close();

        std::cout<<"File " << filename << " created..." << std::endl;
    }
}