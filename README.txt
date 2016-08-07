1) Directory structure is as follows(made by tree command):
	.
	|-- makefile
	|-- README.txt
	|-- results.jpg
	|-- server.cpp
	|-- user.cpp
	`-- worker.cpp

	0 directories, 6 files

2) No extra config files used, only relevant opensource headers used, can be 
	seen in the source code of respective files.

3) Compiling and running:
	Compiling:
	make > makes executables of server, user and worker.
	make clean > removes the executables server, user and worker if at all 
					 present in the current directory
	make server > makes executable only for server
	make user > makes executable only for user
	make worker > makes executable only for worker

	The standard sequence to run and test:

	1.	make
	2.	./server <server-port>
	3.	./worker server <ip/host-name> <server-port> 
	4.	**The above command is applied as many times as required no.of workers **
	5.	./user <server ip/host-name> <server-port> <hash> <passwd-length> <binary-string>

	In first command we compile and create all the executables i..e server, user, worker.
	Second command sets up the server on the machine at the specified port if thats allowed.
	Third command sets up the worker and it connects to the server and waits for instructions.
	Fifth command sets up the user, sends the server the info of hash etc shown and waits for the password. 

	Now the server after receiving the instructions from user, sees the number of workers 
	online and divides the workload among them as follows:

	If there are n workers and see how many of three sets of characters (small alphabet, 
	capital alphabet, numbers) are there by seeing the binary number sent by user. Then 
	the search space of the first letter is divided into n parts. Each worker is allocated 
	work with the first letter being restricted to a fixed search space.

	Then the workers brute force along these instructions from server and return the answer 
	if they get else they will reply that it was not found.

	Then the server forward the result to the user and the user shows how much time in 
	millisecs did it take to crack the password.
