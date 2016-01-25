#include <iostream>
#include <stdio.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <pthread.h>

using namespace std;

#define CONN "Connection"
#define CRLF "\r\n"
#define SP ' '
#define NF "404"
#define OK "200"
#define BD "400"
#define NFS "Not Found"
#define OKS "OK"
#define BDS "Bad Request"
#define NFCONTENT "404 File Not Found"
#define PORT 30007
#define CLOSE "close"
#define ALIVE "keep-alive"
#define GET "GET"

class HTTP {

public:
	string Method;
	string URI;
	string Http_version;
	string connection;
	bool parse_request(string request);
	void get_request(string line);
	void get_connection(string line);
	string prepare_response(string x, string content,bool alive);
	HTTP();
private:
	void bad_request();

};

HTTP::HTTP(){}

void HTTP::get_request(string line){

	int split_pos = line.find(SP);
	
	this->Method = line.substr(0,split_pos);
	line = line.substr(split_pos+1);
	split_pos = line.find(SP);
	
	this->URI = line.substr(0,split_pos);
	line = line.substr(split_pos+1);
	
	this->Http_version = line;
}

void HTTP::get_connection(string line){
	int split_pos = line.find(CONN);
	this->connection = line.substr(split_pos+12);
}

string HTTP::prepare_response(string x,string content,bool alive){
	ostringstream r_stream;
	int content_length;

	r_stream << this->Http_version << SP;

	if(x.compare(NF) == 0){
		r_stream << NF << SP << NFS << CRLF;
		content = NFCONTENT;
	}else if(x.compare(BD) == 0){
		r_stream << BD << SP << BDS << CRLF;
	}else{
		r_stream << OK << SP << OKS << CRLF;
	}

	r_stream << "Content-Length:" << SP << content.length() << CRLF;

	if(alive){
		r_stream << "Connection:" << SP << ALIVE << CRLF;
	}else{
		r_stream << "Connection:" << SP << CLOSE << CRLF;
	}

	r_stream << CRLF;
	
	if( x.compare(NF) == 0){
		r_stream << NFCONTENT << CRLF;	
	}else if(x.compare(BD) != 0 && x.compare(NF) != 0){
		r_stream << content << CRLF;
	}

	string response = r_stream.str();
	r_stream.str(string());
	return response;
}

bool HTTP::parse_request(string request){
	int pos = request.find(CRLF);
	string line = request.substr(0,pos);

	for(int i=0;line.length() > 0;i++){
		if(i == 0){
			// Request Line
			this->get_request(line);
			
		}else if(line.find(CONN) != -1){
			// Request Header to find connection
			this->get_connection(line);
		}

		request = request.substr(pos+2);
		pos = request.find(CRLF);
		line = request.substr(0,pos);
	}

	if(this->Method.compare(GET) != 0){
		return false;
	}
	return true;
}

class ServerSock {
	int server_sock_fd;
	int port_number;
public:
	~ServerSock();
	ServerSock(int domain,int type,int protocol,int port);
	int get_socket();
	string read_buffer(int new_socket_fd);
	int listen_server();
private:
	int socket_server(int domain, int type, int protocol);
	int bind_server(); 
};

int ServerSock::get_socket(){
	return this->server_sock_fd;
}

ServerSock::ServerSock(int domain,int type,int protocol,int port){
	this->port_number = port;
	this->server_sock_fd = -1;
	this->socket_server(domain,type,protocol);
	if(this->bind_server() > 0){
		cout << "Binding successfully done...." << endl;
	}
}

ServerSock::~ServerSock() {
	close (this->server_sock_fd);
}

int ServerSock::socket_server(int domain, int type, int protocol) {

	this->server_sock_fd = socket(domain, type, protocol);

	if (this->server_sock_fd < 0) {
		cerr << "Error in opening socket: " << strerror(errno) << endl;
		exit(0);
	}

	return 1;
}

int ServerSock::bind_server(){

	struct sockaddr_in server_address;
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(this->port_number);
	
	if( bind(this->server_sock_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0 ){
		cerr << "Error in binding: " << endl;
		cout << "Error Code: " << strerror(errno) << endl;
		exit(1);
	}

	return 1;
}

int ServerSock::listen_server(){

	struct sockaddr_in client_addr;
	int new_sock_fd;
	socklen_t client_length;
	
	listen(this->server_sock_fd, 5);
	client_length = sizeof(client_addr);
	new_sock_fd = accept(this->server_sock_fd, (struct sockaddr *) &client_addr, &client_length);

	if(new_sock_fd < 0){
		cerr << "Error in Accepting: " << endl;
		cout << "Error Code: " << strerror(errno) << endl;
		exit(2);
	}

	return new_sock_fd;
}

string get_response_stream(HTTP *http){
	ifstream input_file;
	string response_string;
	input_file.open(http->URI.substr(1).c_str());
	string data;
	ostringstream c_stream;
		
	if(!input_file.is_open()){
		// Unable to open file = 404 not found
		response_string = http->prepare_response(NF,"",false);
	}else{
		// Able to open file = 200 OK
		while(getline(input_file,data)){
			c_stream << data;
		}
		response_string = http->prepare_response("",c_stream.str(),true);
		input_file.close();
	}
	c_stream.str(string());
	data.clear();

	return response_string;
}

void *run(void *argument){

	int client_fd;
	char buffer[1024];
	int n;
	
	bzero(buffer,1024);
	client_fd = * (int *)argument;
	bool alive = true;

	cout << "New Thread Started With Client FD: " << client_fd << endl << endl;

	while ( (n = read(client_fd,buffer,1024)) > -1 ){

		if( n > 0){
			cout << "Request from Client: " << endl << buffer << endl;
			
			HTTP* http = new HTTP();
			string response_string;
			if( http->parse_request(buffer) ){
				
				// Success Parsing the request
				if(http->connection.compare(CLOSE) == 0){
					alive = false;
				}

				response_string = get_response_stream(http);

			}else{

				// Failure Parsing the request
				response_string = http->prepare_response(BD,"",false);
				alive = false;
			}

			write(client_fd,response_string.c_str(),response_string.length());

			bzero(buffer,1024);

		}

		if(!alive){
			cout << "Thread with client FD closed: " << client_fd << endl;
			break;
		}

	}

	if ( n < 0){
		cerr << "Abrupt Close of Client Socket" << endl;
		exit(5);
	}
	cout << "Client Socket FD closed" << endl;
	close(client_fd);
}

int main() {
	ServerSock* server = new ServerSock(AF_INET, SOCK_STREAM, 0, PORT);
	int new_socket_fd;
	pthread_t thread1[9999];
	int thread_count = 0;
	int i;
	cout << "Main Server FD:" << server->get_socket() << endl;
	
	while( (new_socket_fd = server->listen_server()) > 0){
		
		// Pass new_socket_fd to thread.
		i = pthread_create(&thread1[thread_count], NULL, run, (void*) &new_socket_fd );
		pthread_join(thread1[thread_count],NULL);
		thread_count += 1;

		if(thread_count == 9998){
			cout << "Thread count has reached its limit" << endl;
			exit(3);
		}

		if(i){
			cerr << "Unable to create thread" << endl;
			exit(4);
		}

	}

	close(new_socket_fd);
}