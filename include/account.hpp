#include "structures.hpp"

//requires structures.hpp for the position struct

//

class Account{
    public:

        //types of instantiation
        //1. only starting balance included
        //2. ask for initial positions
        Account(double initBalance);

        Account(double initBalance, bool initPos);

        //helper functions
        double checkBalance();
        void modifyBalance(double modifier);

        void addNewPosition(Position newPos);
        void addPositionQuantity(Position targetPos, long quantityChange, double entryPrice);
        void subtractPositionQuantity(Position targetPos, long quantityChange, double entryPrice);
        void removePosition(Position targetPos);

        double positionAEP(Position targetPos); //AEP => Average Entry Price
        long positionQuantity(Position targetPos); 

    private:
        double balance = 10000.0;
        
        
}