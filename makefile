.PHONY: all clean server worker user
all: server worker user
server:
	@g++ -std=c++11 server.cpp -o server
user:
	@g++ -std=c++11 user.cpp -o user
worker:
	@g++ -std=c++11 worker.cpp -o worker -lcrypt
clean:
	@rm -rf user server worker 2>/dev/null
