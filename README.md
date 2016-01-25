README:

There are 4 folders in this assignment
TCP Server & Client
UDP Server & Client

All folders have its own makefile.

Server will not close once the persistent requests has been sent. You can send another request of persistence or non-persistence with the server running.

To run a TCP Server:
1. go to Server_TCP
2. make clean, make all
3. ./server_tcp

To run a TCP Client:
1. go to Client_TCP
2. make clean, make all
3. persistent: ./client_tcp localhost 30007 p file_list.txt
4: non-persistent: ./client_tcp localhost 30007 np lorem_ipsum.txt

To run a UDP Server:
1. go to Server_UDP
2. make clean, make all
3. ./server_udp

To run a UDP Client:
1. go to Client_UDP
2. make clean, make all
3. ./client_udp localhost 30008 lorem_ipsum.txt