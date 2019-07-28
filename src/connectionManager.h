#ifndef __CONNECTIONMANAGER_H_INCLUDED__
#define __CONNECTIONMANAGER_H_INCLUDED__


#define LOCALHOST "0.0.0.0"
#define LISTENINGPORT 9090
#define DEBUG true
#define BLOCK_SIZE 1024
#define NO_OF_THREADS 10
#define min(a,b) a<b ? a : b
#define BROADCAST_IP "255.255.255.255"
#define BROADCAST_PORT 9091
#define REMOTE "127.0.0.1"

using namespace std;

class ConnectionManager{
	private:
		unsigned int socket_descriptor;
		bool is_tcp;
		unsigned int listening_port;
		unsigned int remote_port;
		string self_ip_address;
		string remote_ip_address;
		unsigned int number_of_connections;
		char* buffer;
		char* data;
	public:
		ConnectionManager(bool is_tcp=true, unsigned int listening_port=LISTENINGPORT, string ip_address=LOCALHOST);		
		void init();
		void startServer(unsigned int number_of_connections=SOMAXCONN);
		string readDatagram();		
		int acceptConnection();
		void connectToServer(string remote_ip_address=LOCALHOST, unsigned int remote_port=LISTENINGPORT);
		void sendDataToServer(char* data);
		void sendDatagramToServer(char* data, string remote_ip_address=REMOTE, unsigned int remote_port=LISTENINGPORT);
		void getServerDetails();
		void getClientDetails();	
		void broadcast(char* data);	
		void split(string s, vector<string>&v, string delimiters);
};

#endif