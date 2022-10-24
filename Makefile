
tm_interpreter: tm_interpreter.cpp turing_machine.cpp turing_machine.h 
	g++ -Wall -Wshadow -std=c++2a $(filter %.cpp,$^) -o $@

clean:
	rm -rf tm_interpreter *~