build:init
	g++ -O3 -std=c++11 gps2traj.cpp -o bin/gps2traj
init:
	mkdir -p bin
install:
	cp bin/gps2traj /usr/local/bin
