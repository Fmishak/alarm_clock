//
//  main.cpp
//  alarm-clock
//
//  Created by Farouk Mishak on 11/12/18.
//  Copyright © 2018 Farouk Mishak. All rights reserved.
//

#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include<unistd.h>
#include <time.h>
#include<iomanip>
#include <stdlib.h>
using namespace std;




int main() {

    while (1){
        time_t current = time(0);
        cout << ctime(&current)<< endl;
  
        sleep(1);   //make the program sleep for 1 second
       
    
    }
   
    return 0;
    
    
}
