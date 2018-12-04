.POSIX:

.PHONY: all clean

# Type "make" to download Cereal and compile the program:
all: alarm-clock

alarm-clock: main.cpp
	# If cereal does not exist, then run "git clone ..." to get it.
	test -e cereal || git clone https://github.com/USCiLab/cereal.git cereal
	g++ -Wall -std=c++11 -Icereal/include main.cpp -lncurses -o alarm-clock
