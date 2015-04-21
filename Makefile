main: main.cpp 
	g++ -ggdb3 -pedantic -pthread -o main main.cpp 
clean:
	rm -f main *~  
	rm -r *.dSYM
