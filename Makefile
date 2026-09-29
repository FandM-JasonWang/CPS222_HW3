.PHONY: all clean

all:
	g++ -std=c++17 -O3 -flto -Wall -Wextra main.cpp -o 3d-floating-shape -lX11

clean:
	rm -f 3d-floating-shape *.o