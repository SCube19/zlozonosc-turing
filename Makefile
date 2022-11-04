
tm_converter: tm_converter.cpp turing_machine.cpp turing_machine.h 
	g++ -Wall -Wshadow -std=c++2a $(filter %.cpp,$^) -o $@

clean:
	rm -rf tm_interpreter *~