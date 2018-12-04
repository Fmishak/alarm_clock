//
//  main.cpp
//  alarm-clock
//
//  Created by Farouk Mishak on 11/12/18.
//  Copyright © 2018 Farouk Mishak. All rights reserved.
//
// Compile with: g++ -Wall main.cpp -lncurses -o alarm-clock
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
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
// Get a copy of the Cereal library with:
// git clone https://github.com/USCiLab/cereal.git cereal
// Include Cereal's support for C++ string and vector
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
// Cereal supports archives in several formats.
// Tried JSON since it is compact and human-readable, but it was making compiler warnings:
//#include <cereal/archives/json.hpp>
// Using Cereal's "binary" format instead:
#include <cereal/archives/binary.hpp>

using namespace std;

void start_ncurses() {
    // Finish setting up our screen since we know that TERM is set:
    initscr(); // tell ncurses to initialize itself.
    clear(); // clear the screen. see "man clear"
    noecho(); // If a user presses a key, don't automatically let the terminal show it. see "man noecho"
    cbreak(); // see "man cbreak"
    timeout(100); // getch() should wait 100 ms for a key to be pressed.  See "man timeout"
}

void stop_ncurses() {
    // turn off ncurses to let cin work properly
    clear();
    refresh(); // send the echo and clear commands to the terminal
    endwin();  // reset ncurses and return terminal to normal
}

struct ALARM {
    unsigned hour, minute;
    string name;

    // Cereal library needs this to know what variables we want to save and load:
    template<class Archive>
    void serialize(Archive & archive) {
        archive(hour, minute, name); // list of our struct's variables
    }
};

void save_alarms(vector<ALARM> alarms, string filename) {
    ofstream out(filename, ios::binary);
    cereal::BinaryOutputArchive archive(out);
    archive(alarms);
}

vector<ALARM> load_alarms(string &filename) {
    vector<ALARM> alarms;
    ifstream in(filename, ios::binary);
    if (!in.is_open())
        return alarms; // return empty vector if we could not open the file.
    cereal::BinaryInputArchive archive(in);
    archive(alarms);
    return alarms;
}

void show_alarms(vector<ALARM> alarms, char *term_type, int start_y, int start_x) {
    clear();
    if (term_type)
        mvprintw(start_y++, start_x, "#  HH:MM  Name");
    else
        cout << "#  HH:MM  Name\n";
    for (int n = 0; n<alarms.size(); n++) {
        ALARM alarm = alarms[n];
        if (term_type)
            mvprintw(start_y++, start_x, "%i  % 2u:%02u  %s\n", n, alarm.hour, alarm.minute, alarm.name.c_str());
        else
            cout << n << "  " << setw(2) << alarm.hour << ':' << setfill('0') << setw(2) << alarm.minute << setfill(' ') << "  " << alarm.name << "\n";
    }
}

vector<ALARM> add_alarm(vector<ALARM> alarms, char *term_type) {
    if(term_type) { // turn off ncurses for cin to work
        stop_ncurses();
    }
    // TODO:
    // Ask user for hour, minute, and description:
    ALARM alarm;
    // TODO:  put some cout's in here to be friendly
    cout << "\nPlease enter an alarm (hour, minute, name)\n";
    cout << "Hour: ";
    cin >> alarm.hour;
    cout << "Minute: " ;
    cin >> alarm.minute;
    cout << "Name: ";
    cin >> alarm.name;
    if(term_type) { // turn on ncurses again
        start_ncurses();
    }

    alarms.push_back(alarm);
    return alarms;
}

vector<ALARM> delete_alarm(vector<ALARM> alarms, char *term_type) {
    if(term_type) { // turn off ncurses for cin to work
        stop_ncurses();
    }
    cout << "Which alarm # do you want to delete?\n";
    show_alarms(alarms, 0, 3, 10); // using 0 instead of term_type, because we turned off ncurses for cin to work.
    unsigned number;
    cin >> number;
    if (number>=alarms.size()) {
        cout << "Invalid alarm # " << number << endl;
        sleep(3); // Let user see our message for 3 seconds before going on.
    }
    if(term_type) { // turn on ncurses again
        start_ncurses();
    }
    // Tell C++ vector to erase the alarm:
    alarms.erase(alarms.begin() + number);
    return alarms;
}

void show_help(int start_y, int start_x) {
    clear();
    mvprintw(start_y++, start_x, "Key  Action");
    mvprintw(start_y++, start_x, " A   Add alarm");
    mvprintw(start_y++, start_x, " D   Delete alarm");
    mvprintw(start_y++, start_x, " L   List alarms");
    mvprintw(start_y++, start_x, " ?   Show this help screen");
    mvprintw(start_y++, start_x, "SPACE  Silence an active alarm");
    refresh(); // actually send the text to the screen!
}

int wrap_24_hours(int seconds_since_midnight) {
    // Does C/C++ % operator handle negative numbers correctly?
    // until we verify, let's manually add a day at a time until the number is positive:
    while(seconds_since_midnight < 0)
        seconds_since_midnight += 24 * 60 * 60;
    // Now divide and keep the remainder so that we naturally wrap every 24 hours:
    seconds_since_midnight %= 24 * 60 * 60;
    return seconds_since_midnight;
}

int seconds_between(unsigned alarm, unsigned start_time, unsigned end_time) {
    // duration is how many seconds since start_time that we want to look for any alarms:
    int duration = end_time - start_time;
    int x = wrap_24_hours(alarm - start_time);
    if (x>=0 && x<=duration)
        return 1;
    else
        return 0;
}

string get_new_ringing_alarm(vector<ALARM> alarms, time_t old_time, time_t current_time, int *alarm_is_ringing) {

    struct tm *old_date_time = localtime(&old_time); // man localtime
    struct tm *current_date_time = localtime(&current_time); // man localtime
    unsigned old_seconds_since_midnight =
        old_date_time->tm_hour * 3600 +
        old_date_time->tm_min * 60 +
        old_date_time->tm_sec; // ignoring tm_year, tm_mon, and tm_mday
    unsigned current_seconds_since_midnight =
        current_date_time->tm_hour * 3600 +
        current_date_time->tm_min * 60 +
        current_date_time->tm_sec; // ignoring tm_year, tm_mon, and tm_mday

    string name_of_ringing_alarm;
    for (int n = 0; n<alarms.size(); n++) {
        ALARM alarm = alarms[n];

        unsigned alarm_seconds_since_midnight = alarm.hour * 3600 + alarm.minute * 60;
        // TODO: Is alarm_seconds_since_midnight between old_seconds_since_midnight and current_seconds_since_midnight
        if (seconds_between(alarm_seconds_since_midnight, old_seconds_since_midnight, current_seconds_since_midnight)) {
            *alarm_is_ringing = 1;
            name_of_ringing_alarm = alarm.name;
        }
    }
    return name_of_ringing_alarm;
}
//get_new_ringing_alarm will set alarm_is_ringing=1 if a new alarm is ringing.

int main() {
    time_t old_time;
    time(&old_time); // when we first start, skip any alarm that has already passed.
    // we'll only care about alarms that happen while we're running.

    string filename = "alarms.cereal";
    vector<ALARM> alarms;
    alarms = load_alarms(filename);

    char *term_type = getenv("TERM"); // This will return 0 if TERM isn't defined in the "environment"

    if(term_type){
        start_ncurses();
    }
    
    int alarm_is_ringing = 0;
    string name_of_ringing_alarm;

    int done = 0;
    while (!done){
        time_t current_time;
        time(&current_time); // get time in seconds since Jan 1, 1970, midnight.  See "man 3 time"
        struct tm *current_date_time = localtime(&current_time); // man localtime
        // get a copy of the hours, minutes, and seconds from the struct to make our code easier below:
        int hour = current_date_time->tm_hour;
        int minute = current_date_time->tm_min;
        int second = current_date_time->tm_sec;
 
        {
        // Check if any new alarm is going off
            time(&current_time);
            int new_alarm_is_ringing = 0;
            string name_of_new_ringing_alarm = get_new_ringing_alarm(alarms, old_time, current_time, &new_alarm_is_ringing);
            if (new_alarm_is_ringing) {
                // Replace any alarm already ringing with the new one we just found:
                alarm_is_ringing = new_alarm_is_ringing;
                name_of_ringing_alarm = name_of_new_ringing_alarm;
                old_time = current_time;
            }
        //get_new_ringing_alarm will set alarm_is_ringing=1 if a new alarm is ringing.
        }

        if(term_type) {
            if (alarm_is_ringing) {
                cerr << "\x07";
                // TODO make flashing color;
                mvprintw(2, 10, "%2i:%02i:%02i", hour, minute, second); // man mvprintw
                mvprintw(3, 10, "ALARM!  name=\"%s\"", name_of_ringing_alarm.c_str()); // man mvprintw
                clrtoeol();
            } else {
                mvprintw(2, 10, "%2i:%02i:%02i", hour, minute, second); // man mvprintw
                clrtoeol();
            }
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
            case 'a':
            case 'A':
                alarms = add_alarm(alarms, term_type);
                save_alarms(alarms, filename);
                break;
            case 'd':
            case 'D':
                alarms = delete_alarm(alarms, term_type);
                save_alarms(alarms, filename);
                break;
            case 'l':
            case 'L':
                show_alarms(alarms, term_type, 5, 10);
                break;
            case ' ':
                // silence alarm:
                alarm_is_ringing = 0;
                break;
            case 'h':
            case 'H':
            case '?':
                show_help(5, 10);
                break;
            case 'q':
            case 'Q':
                done = 1; // we'll exit when the user presses q
                break;
            case ERR:
                // Nothing was pressed. So do nothing.
                break;
            default:
                // Unknown key...?   Do nothing.
                clear();
                mvprintw(3, 10, "Press ? for help.");
                clrtoeol();
                refresh();
                break;
        }
        
        
    }
    if(term_type)
        stop_ncurses();
    
    return 0;
}
