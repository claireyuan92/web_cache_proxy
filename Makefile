main: main.cpp csapp.cpp 
	g++ -ggdb3 -pedantic -pthread -o main main.cpp csapp.cpp
clean:
	rm -f main *~