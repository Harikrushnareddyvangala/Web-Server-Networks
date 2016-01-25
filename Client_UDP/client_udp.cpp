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

#define PORT 30008
#define GET "GET"
#define SP ' '
#define BK '/'
#define VERSION "HTTP/1.1"
#define CRLF "\r\n"
#define CONLEN "Content-Length:"

class ClientSockUDP{
	int client_sock_fd;
public:
	int port_number;
	string server_host;

	~ClientSockUDP();
	ClientSockUDP(int domain,int type,int protocol,int port,string server);
	int socket_client(int domain,int type,int protocol);
	struct sockaddr_in setup_server();
	int get_socket();
};

int ClientSockUDP::socket_client(int domain,int type,int protocol){

	this->client_sock_fd = socket(domain,type,protocol);

	if(this->client_sock_fd < 0){
		cerr << "Error in opening socket: " << strerror(errno) << endl;
		exit(2);
	}

	return 1;
}

ClientSockUDP::ClientSockUDP(int domain,int type,int protocol,int port,string server){
	this->port_number = port;
	this->client_sock_fd = -1;
	this->server_host = server;

	this->socket_client(domain,type,protocol);
}

struct sockaddr_in ClientSockUDP::setup_server(){

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

	return server_address;
}

int ClientSockUDP::get_socket(){
	return this->client_sock_fd;
}

ClientSockUDP::~ClientSockUDP() {
	close (this->client_sock_fd);
}

string prepare_request(string filename){
	ostringstream r_stream;
	
	r_stream << GET << SP << BK << filename << SP << VERSION << CRLF;

	r_stream  << CRLF << CRLF;
	string request = r_stream.str();
	r_stream.str(string());

	return request;
}

int main(int argc, char const *argv[])
{
	string server_host;
	int port,n;
	string file_name;
	struct sockaddr_in server_details;
	char buffer[1024];
	bzero(buffer,1024);
	ostringstream r_stream;
	struct timeval time;
	struct timezone tz;

	if( argc != 4){
		// Client hostname portnumber filename
		cout << "Not Enough Arguments, Please enter arguments in format:" << endl;
		cout << "client server_host/ip server_port connection_type filename.txt" << endl;
		exit(0);

	}else if( argc == 4){

		server_host = argv[1];
		port = atoi(argv[2]);
		file_name = argv[3];
	}
	
	ClientSockUDP* client = new ClientSockUDP(AF_INET, SOCK_DGRAM, 0, PORT,server_host);
	server_details = client->setup_server();

	string request = prepare_request(file_name);
	
	sendto(client->get_socket(),request.c_str(),request.length(),0,(struct sockaddr *)&server_details,sizeof(server_details));
	int bytes = 0;

	while(1){

		n = recvfrom(client->get_socket(),buffer,sizeof(buffer),0,NULL,NULL);
		bytes += n;
		r_stream << buffer;
		if(n < 1024){
			// Last Packet 
			break;
		}
	}

	string response = r_stream.str();
	r_stream.str(string());

	cout << "Response for file: " << file_name << endl;
	cout << response << endl;
	cout << "Number of Bytes:" << bytes << endl;
	gettimeofday(&time, &tz);
	cout << "Time for last byte: " << time.tv_usec << endl;

	return 0;
}