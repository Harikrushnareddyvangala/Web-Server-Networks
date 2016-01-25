#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <string>
#include <ctime>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <errno.h>

using namespace std;

#define NP "np"
#define P "p"
#define PORT 30007
#define GET "GET"
#define SP ' '
#define BK '/'
#define VERSION "HTTP/1.1"
#define CRLF "\r\n"
#define CLOSE "close"
#define ALIVE "keep-alive"
#define CONN "Connection:"
#define CONLEN "Content-Length:"

class ClientSock{
	int client_sock_fd;
public:
	int port_number;
	string server_host;

	~ClientSock();
	ClientSock(int domain,int type,int protocol,int port,string server);
	int socket_client(int domain,int type,int protocol);
	int connect_server();
	int send_request(string request);
	string recieve_response(int size);
};

ClientSock::ClientSock(int domain,int type,int protocol,int port,string server){
	this->port_number = port;
	this->client_sock_fd = -1;
	this->server_host = server;

	this->socket_client(domain,type,protocol);
	
	if(this->connect_server() > 0){
		cout << "Connection successfully established...." << endl;
	}
}

ClientSock::~ClientSock() {
	close (this->client_sock_fd);
}

int ClientSock::socket_client(int domain,int type,int protocol){

	this->client_sock_fd = socket(domain,type,protocol);

	if(this->client_sock_fd < 0){
		cerr << "Error in opening socket: " << strerror(errno) << endl;
		exit(2);
	}

	return 1;
}

int ClientSock::connect_server(){

	struct sockaddr_in server_address;

	if(inet_addr(this->server_host.c_str()) == -1){
		// hostname to ip address
		struct hostent *host;

		if( (host = gethostbyname(this->server_host.c_str()) ) == NULL )
		{
			cerr << "Error resolving hostname " << this->server_host << endl;
			cout << "Error Code: " << strerror(errno) << endl;
			exit(3);
		}

		struct in_addr **address_list;

		address_list = (struct in_addr **) host->h_addr_list;
		
		for( int i = 0; address_list[i] != NULL; i++ ){
			server_address.sin_addr = *address_list[i];
			break;
		}

	}else{
		// plain ip address
		server_address.sin_addr.s_addr = inet_addr(this->server_host.c_str());
	}
	
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(this->port_number);

	if(connect(this->client_sock_fd,(struct sockaddr *) &server_address, sizeof(server_address)) < 0){
		cerr << "Error in connecting: " << endl;
		cout << "Error Code: " << strerror(errno) << endl;
		exit(4);
	}

	return 1;
}

string prepare_request(string filename,bool persistent){
	ostringstream r_stream;
	
	r_stream << GET << SP << BK << filename << SP << VERSION << CRLF;
	r_stream << CONN << SP;

	// Check for persistent request.
	if(persistent)
		r_stream << ALIVE;
	else
		r_stream << CLOSE;

	r_stream  << CRLF << CRLF;
	string request = r_stream.str();
	r_stream.str(string());

	return request;
}

int ClientSock::send_request(string request){
	if(send(this->client_sock_fd,request.c_str(),strlen(request.c_str()),0) < 0){
		cerr << "Error in sending request" << endl;
		exit(5);
	}

	return 1;

}

string parse_response(string response){
	int pos = response.find(CRLF);
	string line = response.substr(0,pos);
	int length = -1;
	for(int i = 0; line.length() > 0; i++){
		if(line.find(CONLEN) != -1){
			string temp = line.substr(16);
			length = atoi(temp.substr(0,-1).c_str());
		}

		// This will take till end of file
		response = response.substr(pos+2);
		pos = response.find(CRLF);
		line = response.substr(0,pos);
	}
	// Skipping CRLF (/r/n)
	response = response.substr(2,length);
	return response;
}

string ClientSock::recieve_response(int size=256){
	char buffer[size];

	ostringstream r_stream;
	
	while( recv(this->client_sock_fd,buffer,sizeof(buffer),0) ){
		r_stream << buffer;
		cout << buffer;
		if(sizeof(buffer) < size){
			break;
		}
	}

	string response = r_stream.str();
	r_stream.str(string());
	return parse_response(response);
}

int main(int argc, char *argv[]){
	string server_host;
	int port;
	bool persistent = true;
	string file_name;
	string persistent_arg;

	if( argc != 5){

		cout << "Not Enough Arguments, Please enter arguments in format:" << endl;
		cout << "client server_host/ip server_port connection_type filename.txt" << endl;
		exit(0);

	}else if( argc == 5){

		server_host = argv[1];
		port = atoi(argv[2]);
		persistent_arg = argv[3];
		if( persistent_arg.compare(NP) == 0 ) persistent = false;
		else if( persistent_arg.compare(P) == 0 ) persistent = true;
		else{
			cerr << "Error in argument for connection type" << endl;
			exit(1);
		}
		file_name = argv[4];
	}
	
	ClientSock* client = new ClientSock(AF_INET, SOCK_STREAM, 0, PORT,server_host);

	if (persistent)
	{
		// Persistent Connection, 
		// read the list of filenames from the filename.txt
		// read the response and printout the response.

		ifstream input_file;
		input_file.open(file_name.c_str());

		if(!input_file.is_open()){
			// Unable to open file
			cerr << "Error in opening the file" << endl;
			exit(6);
		}else{
			// Success opening the file
			string data;
			ostringstream c_stream;

			while(getline(input_file,data)){
				string request;
				
				if(input_file.eof()){
					// Check for last request
					request = prepare_request(data,!persistent);
				}else{
					// Send persistent request
					request = prepare_request(data,persistent);
				}

				client->send_request(request);
				cout << "Contents of File: " << data << endl;
				cout << client->recieve_response((1500)) << endl << endl;
			}
			c_stream.str(string());
			input_file.close();

		}

	}else{
		// Non-Persistent Connection, 
		// send the filename as the request to server.
		// Print the output

		string request = prepare_request(file_name,persistent);
		client->send_request(request);

		cout << "Contents of File: " << file_name << endl;
		cout << client->recieve_response((1500)) << endl;

	}

}