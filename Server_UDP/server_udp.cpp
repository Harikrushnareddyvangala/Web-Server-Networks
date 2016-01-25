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
#include <sys/time.h>

using namespace std;

#define CRLF "\r\n"
#define SP ' '
#define NF "404"
#define OK "200"
#define BD "400"
#define NFS "Not Found"
#define OKS "OK"
#define BDS "Bad Request"
#define NFCONTENT "404 File Not Found"
#define PORT 30008
#define GET "GET"

class HTTP {

public:
	string Method;
	string URI;
	string Http_version;
	bool parse_request(string request);
	void get_request(string line);
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

bool HTTP::parse_request(string request){
	int pos = request.find(CRLF);
	string line = request.substr(0,pos);

	for(int i=0;line.length() > 0;i++){
		if(i == 0){
			// Request Line
			this->get_request(line);
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

class ServerSockUDP {
	int server_sock_fd;
	int port_number;
public:
	~ServerSockUDP();
	ServerSockUDP(int domain,int type,int protocol,int port);
	int get_socket();
private:
	int socket_server(int domain, int type, int protocol);
	int bind_server(); 
};

ServerSockUDP::ServerSockUDP(int domain,int type,int protocol,int port){
	this->port_number = port;
	this->server_sock_fd = -1;
	this->socket_server(domain,type,protocol);
	if(this->bind_server() > 0){
		cout << "Binding successfully done...." << endl;
	}
}

int ServerSockUDP::get_socket(){
	return this->server_sock_fd;
}

int ServerSockUDP::socket_server(int domain, int type, int protocol) {

	this->server_sock_fd = socket(domain, type, protocol);

	if (this->server_sock_fd < 0) {
		cerr << "Error in opening socket: " << strerror(errno) << endl;
		exit(0);
	}

	return 1;
}

int ServerSockUDP::bind_server(){

	struct sockaddr_in server_address;
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
	server_address.sin_port = htons(this->port_number);
	
	if( bind(this->server_sock_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0 ){
		cerr << "Error in binding: " << endl;
		cout << "Error Code: " << strerror(errno) << endl;
		exit(1);
	}

	return 1;
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

int main(){
	ServerSockUDP* server_udp = new ServerSockUDP(AF_INET, SOCK_DGRAM , 0, PORT);
	int n;
	socklen_t client_length;
	struct sockaddr_storage client_details;
	char buffer[1024];
	bzero(buffer,1024);
	struct timeval time;
	struct timezone tz;

	cout << "Main UDP Server FD:" << server_udp->get_socket() << endl;
	client_length = sizeof(client_details);
	while(1){

		n = recvfrom(server_udp->get_socket(),buffer,1024,0,(struct sockaddr *)&client_details,&client_length);

		// Recieve the buffer, parse request, send response
		HTTP* http = new HTTP();
		string response_string;

		cout << "Request from client: " << endl;
		cout << buffer << endl;
		if( http->parse_request(buffer) ){
			
			// Success Parsing the request
			response_string = get_response_stream(http);

		}else{

			// Failure Parsing the request
			response_string = http->prepare_response(BD,"",false);
		}

		bzero(buffer,1024);

		if(response_string.length() > 1024){
			int n_packets = response_string.length() / 1024;
			int last_packet = response_string.length() % 1024;
			string temp;
			int counter = 1;

			while(counter != n_packets){
				temp = response_string.substr(0,1024);
				response_string = response_string.substr(1025);

				sendto(server_udp->get_socket(),temp.c_str(),temp.length(),0,(struct sockaddr *)&client_details,sizeof(client_details));
				counter += 1;
				temp.clear();
			}

			if(last_packet > 0){
				sendto(server_udp->get_socket(),response_string.c_str(),response_string.length(),0,(struct sockaddr *)&client_details,sizeof(client_details));
				gettimeofday(&time, &tz);
				cout << "Time for last byte: " << time.tv_usec << endl;
			}

		}else{
			sendto(server_udp->get_socket(),response_string.c_str(),response_string.length(),0,(struct sockaddr *)&client_details,sizeof(client_details));
			gettimeofday(&time, &tz);
			cout << "Time for last byte: " << time.tv_usec << endl;
		}

	}
	
}