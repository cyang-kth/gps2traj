build:init
	g++ -O3 -std=c++11 gps2traj.cpp -o bin/gps2traj
	g++ -O3 -std=c++11 traj2gps.cpp -o bin/traj2gps
init:
	mkdir -p bin
install:
	cp bin/gps2traj /usr/local/bin
	cp bin/traj2gps /usr/local/bin
