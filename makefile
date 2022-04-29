cc = g++
src = ./src/request.cc
prom = ./bin/request

$(prom): $(src)
	$(cc) -std=c++11 $(src) -o $(prom)
clean:
