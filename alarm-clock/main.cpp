//
//  main.cpp
//  alarm-clock
//
//  Created by Farouk Mishak on 11/12/18.
//  Copyright © 2018 Farouk Mishak. All rights reserved.
//
// Compile with: g++ -Wall main.cpp -lncurses -o alarm-clock
#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include<unistd.h>
#include <time.h>
#include<iomanip>
#include <stdlib.h> // getenv
#ifdef WIN32
// Visual Studio will automatically "define" WIN32.
// This will trigger the code to include conio.h instead of curses.h
// TODO: Make sure WIN32 is defined
#include <conio.h>
#else
#include <ncurses.h> // for unix-related systems (likely not MS Windows... for Windows, try <conio.h>)
#endif

using namespace std;




int main() {
    char *term_type = getenv("TERM"); // This will return 0 if TERM isn't defined in the "environment"

    if(term_type){
        // Finish setting up our screen since we know that TERM is set:
        initscr(); // tell ncurses to initialize itself.
        clear(); // clear the screen. see "man clear"
        noecho(); // If a user presses a key, don't automatically let the terminal show it. see "man noecho"
        cbreak(); // see "man cbreak"
        timeout(100); // getch() should wait 100 ms for a key to be pressed.  See "man timeout"
    }
    
    int done = 0;
    while (!done){
        time_t current_time;
        time(&current_time); // get time in seconds since Jan 1, 1970, midnight.  See "man 3 time"
        struct tm *current_date_time = localtime(&current_time); // man localtime
        // get a copy of the hours, minutes, and seconds from the struct to make our code easier below:
        int hour = current_date_time->tm_hour;
        int minute = current_date_time->tm_min;
        int second = current_date_time->tm_sec;
 
        if(term_type) {
            mvprintw(2, 10, "%2i:%02i:%02i", hour, minute, second); // man mvprintw
            clrtoeol();
            refresh(); // actually send the text to the screen!
        } else
            cout << setfill(' ') << setw(2) << hour << ":" << setfill('0') << setw(2) << minute << ":" << setw(2) << second << setfill(' ') << endl;
        
        int key = getch(); // ncurses function to get a character from cin.  see "man getch"
            // getch() will wait for timeout (above) before returning.
        if(!term_type) {
            // getch() will return immediately when $TERM isn't set (such as when running accidentally in Xcode) so we haven't initialized ncurses with initscr().
            // We added this usleep to slow down our code, so that we don't run up the CPU and drain the laptop battery.
            usleep(100000);   //make the program sleep for 100,000 mircoseconds (0.1 seconds)
        }

        switch (key) {
            case 'q':
            case 'Q':
                done = 1; // we'll exit when the user presses q
                break;
            case ERR:
                // Nothing was pressed. So do nothing.
                break;
            default:
                // Unknown key...?   Do nothing.
                break;
        }
        
        
    }
    if(term_type)
        endwin();
    
    return 0;
}
