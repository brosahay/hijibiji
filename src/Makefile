connectionManager:
	g++ -std=c++11 connectionManager.cpp -o connectionManager.out
main:
	g++ -std=c++11 main.cpp utils.cpp metadata.cpp -o main.out -lssl -lcrypto -pthread
debug:
	g++ -g -rdynamic -std=c++11 -Wall main.cpp utils.cpp metadata.cpp -o main.out -lssl -lcrypto -pthread
test:
	g++ -std=c++11 test.cpp utils.cpp -o test.out -lssl -lcrypto -pthread
maincopy:
	g++ -std=c++11 maincopy.cpp utils.cpp -o maincopy.out -lssl -lcrypto -pthread
utils:
	g++ -std=c++11 main.cpp utils.cpp -o utils.out -lssl -lcrypto
sender:
	g++ -std=c++11 sender.cpp -o sender.out -pthread
listener:
	g++ -std=c++11 listener.cpp -o listener.out -pthread
driver:
	g++ -std=c++11 driver.cpp -o driver.out -pthread
clean:
	rm -f *.out
#all: sender.out listener.out
