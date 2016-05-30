#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <atomic>
#include <future>
#include <mutex>
#include <errno.h>
#include <malloc.h>

#include "constants.h"

//extern "C" void* processConnection(void *args);
using namespace std;

mutex mtx;
mutex mtxMeta;
map<string,bool> sourceIPMap;
extern unsigned int downloadThreadNo;
extern int errno;
extern long long int downloaded;

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
		// Vector to handle connection in each thread.
		vector<thread> connectionThreadVector;
		// Counter to keep track of thread number
		unsigned int thread_no;
		//for listening to meta requests in thread 
		vector<thread> metaThreadVector;
		unsigned int metaThreadNo;
		//for listening to block requests in thread 
		vector<thread> blocksThreadVector;
		unsigned int blocksThreadNo;

	public:

		ConnectionManager(bool is_tcp=true, unsigned int listening_port=LISTENINGPORT, string ip_address=LOCALHOST){
			// Initialise the variables
			this->is_tcp = is_tcp;
			this->listening_port = listening_port;
			this->self_ip_address = ip_address;
			this->thread_no = 0;
			this->metaThreadNo =0;
			this->blocksThreadNo=0;
			// Initialise the object
			this->init();
			if(DEBUG){
				cout<<"Socket initialised.\n";
			}
		}

		// Initialise a TCP or UDP socket based on the boolean is_tcp
		void init(){
			if(this->is_tcp){
				if((this->socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1){
					if(DEBUG){
						cout<<"Error while creating TCP socket\n";
						cout<<strerror(errno);
					}
					exit(1);
				}
			}
			else{
				if((this->socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
					if(DEBUG){
						cout<<"Error while creating UDP socket\n";
						cout<<strerror(errno);
					}
					exit(1);
				}
			}

				//Set socket options

			int yes =1;
			int setsockreturn = setsockopt(this->socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
			if(DEBUG)
			{
				cout<<"Set sock returns "<<setsockreturn<<endl;
			}
			if(setsockreturn==-1){
				if(DEBUG){
					cout<<"Error while setting socket options.\n";
					cout<<strerror(errno);
				}
				exit(1);
			}

		}

		// Start a server. TCP or UDP dependent on the socket type.
		void startServer(unsigned int number_of_connections=SOMAXCONN){
			// Initialise the number of connections for the server
			this->number_of_connections = number_of_connections;
			struct sockaddr_in server;

			memset(&server,0,sizeof(server));
			server.sin_family=AF_INET;
			server.sin_addr.s_addr=inet_addr(this->self_ip_address.c_str());
			server.sin_port = htons(this->listening_port);

			// Bind socket to address
			if(bind(this->socket_descriptor, (struct sockaddr*)&server, sizeof(server))!=0){
				if(DEBUG){
					cout<<"Error while binding socket to local address.\n";
					cout<<"Origin : ConnectionManager::startServer()\n";
					cout<<strerror(errno);
				}
				exit(1);
			}

			if(this->is_tcp){
				// Listen for connections
				if(listen(this->socket_descriptor, this->number_of_connections)!=0){
					if(DEBUG){
						cout<<"Error while listening for connections.\n";
						cout<<"Origin : ConnectionManager::startServer()\n";
						cout<<strerror(errno);
					}
					exit(1);
				}
			}

			// Debugging information
			if(DEBUG)
			{
				cout<<"Server started. Waiting for connections.\n";
				cout<<"Listening on port :: "<<this->listening_port<<endl;
				if(this->is_tcp){
					cout<<"Number of connections allowed :: "<<this->number_of_connections<<endl;
				}
			}

			fflush(stdout);
		}

		string readData(){
			char buffer[BLOCK_SIZE];
			bzero(buffer, BLOCK_SIZE+1);
			if((recv(this->socket_descriptor, buffer, BLOCK_SIZE, 0)) < 0 ){
				exit(1);
			}		
			if(DEBUG){
				cout<<"Data received: "<<buffer;		
			}
			return string(buffer);
		}

		//READ DATA FROM UDP
		pair <string,string> readDatagram(){
			struct sockaddr_in remote;
			socklen_t remote_len = sizeof(remote);
			char buffer[BLOCK_SIZE];
			bzero(buffer,BLOCK_SIZE+1);

			if((recvfrom(this->socket_descriptor, buffer, BLOCK_SIZE, 0, (struct sockaddr *)&remote, &remote_len)) < 0){
				exit(1);
			}
			char ipAddress[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(remote.sin_addr), ipAddress, INET_ADDRSTRLEN);		
			if(DEBUG){
				cout<<"Client's IP Address: "<<ipAddress<<endl;			
				cout<<"Data : "<<buffer<<endl;
			}
			pair <string,string> dataDatagram;
			dataDatagram.first=ipAddress;
			dataDatagram.second=buffer;
			if(DEBUG){
				cout<<"Client's IP Address: "<<dataDatagram.first<<endl;			
				cout<<"Data : "<<dataDatagram.second<<endl;
			}
			return dataDatagram;
		}

		// Accept an incoming connection. Return the client socket information to the calling function.
		int acceptConnection(){
			// identify type of message here using port number
			if(this->listening_port==META_REQUEST_PORT){
				acceptMetaRequest();
			}
			else if(this->listening_port==BLOCKS_REQUEST_PORT){
				acceptBlocksRequest();
			}
			return 0;
		}

		void acceptBlocksRequest(){			
			struct sockaddr_in client;
			int new_fd;			
			socklen_t client_size = sizeof(client);
			while(metaThreadNo < NO_OF_THREADS){
				new_fd = accept(this->socket_descriptor, (struct sockaddr*)&client, &client_size);
				if(new_fd < 0){
					if(DEBUG)
						cerr << "Cannot accept connection" << endl;
					exit(1);
				}
				else{
					if(DEBUG){
						cout<<"Connection successful" << endl;
					}
				}
				blocksThreadVector.push_back(thread(&ConnectionManager::sendRequestedBlocks,this,new_fd));				
				mtxMeta.lock();
				blocksThreadNo++;
				mtxMeta.unlock();
			}
			for (auto& th : blocksThreadVector) th.join();								
		}

		void sendRequestedBlocks(int fd){
			char buffer[BLOCK_SIZE];
			bzero(buffer,BLOCK_SIZE+1);
			recv(fd, buffer, BLOCK_SIZE, 0);
			if(DEBUG){
				cout<<"Block Request Received: "<<buffer;
			}
			vector<string> params;
			split((string)buffer, params, ";");
			if(DEBUG){
				cout<<endl;
				cout<<params.size()<<endl;
			}
			string fileName = params[0];
			int startBlock = stoi(params[1]);
			int lastBlock = stoi(params[2]);
			int sizeOfLastBlock = stoi(params[3]);
			ifstream file(fileName,ios::in|ios::binary);
			if(DEBUG){
				cout<<"File status : "<<file.is_open()<<endl;
			}
			if(file.is_open()){
				unsigned int blockSize = BLOCK_SIZE;
				for(int i = startBlock; i <= lastBlock; i++){
					if(i==lastBlock)
						blockSize=sizeOfLastBlock;
					bzero(buffer,BLOCK_SIZE+1);
					file.seekg((i*BLOCK_SIZE), ios::beg);
					file.read(buffer,blockSize);
					if(DEBUG){
						cout<<"Block being sent: "<<buffer<<endl;									
					}
					int status = send(fd, buffer, blockSize, 0);
					if(DEBUG){
						cout<<"Send status : "<<status<<endl;
						if(status == -1){
							cout<<"Operation cancelled by client"<<endl;
						}						
					}
		    }
		    file.close();			
		  }
			if(DEBUG){
				cerr<<strerror(errno);
			}
			mtxMeta.lock();
			blocksThreadNo--;
			mtxMeta.unlock();
			close(fd);
		}
		void acceptMetaRequest(){
		 	struct sockaddr_in client;
		 	int new_fd;			
		 	socklen_t client_size = sizeof(client);
		 	while(metaThreadNo < NO_OF_META_THREADS){
		 		new_fd = accept(this->socket_descriptor, (struct sockaddr*)&client, &client_size);
		 		if(new_fd < 0){
		 			if(DEBUG){
					  cerr << "Cannot accept connection" << endl;
					}
					exit(1);
				}
				else{
					if(DEBUG){
		 				cout<<"Connection successful" << endl;
		 			}
		 		}
				metaThreadVector.push_back(thread(&ConnectionManager::processMetaConnection,this,new_fd));				
				metaThreadNo++;							
			}
			for (auto& th : metaThreadVector) th.join();								
		}

		void processMetaConnection(int fd){
	   	if(DEBUG)
	   		cout<<"Processing meta connection : "<<fd<<endl;
	   	char buffer[BLOCK_SIZE];
	   	bzero(buffer,BLOCK_SIZE+1);
	   	recv(fd, buffer, BLOCK_SIZE, 0);			
	   	if(DEBUG)
	   		cout<<buffer;
	   	string fileName = strdup(buffer);
	   	fileName = fileName+META_EXTENSION+P2P_EXTENSION;
			// Check if meta file is present
	   	ifstream metafile(fileName.c_str());
	   	if(!metafile.good()){
	   		bzero(buffer, BLOCK_SIZE+1);
	   		strcpy(buffer, "META -1");
	   	}
	   	else{
				//send mata file here		
		 		FILE* reader = fopen(fileName.c_str(),"r");			
		 		fflush(stdout);	
		 		fseek(reader, 0, SEEK_END);			
		 		int fileSize = ftell(reader);
		 		fseek(reader, 0, SEEK_SET);	
		 		bzero(buffer,BLOCK_SIZE+1);
		 		if(DEBUG){
					cout<<"\nfileSize: "<<fileSize;
				}
				fread(buffer, 1, fileSize, reader);
				fclose(reader);
			}
	   	if(DEBUG){
				cout<<"Sending this data: "<<buffer<<endl;
				cout<<"strlen: "<<strlen(buffer)<<endl;
			}
			int status = send(fd, buffer, strlen(buffer), 0);
			if(DEBUG)
				cout<<"Send status : "<<status<<endl;			
				metaThreadNo--;
		}		

	  void fetchBlocks(string ip, string fileName, int startBlock, int lastBlock, int sizeOfLastBlock){	
			connectToServer(ip.c_str(),BLOCKS_REQUEST_PORT);
			string dataToSend = fileName+";"+ to_string(startBlock)+";"+ to_string(lastBlock)+";"+to_string(sizeOfLastBlock);
			sendDataToServer((const char*)dataToSend.c_str());            
	   	queue<char> dataQ;			
	   	int totalReqSize=((lastBlock-startBlock)*BLOCK_SIZE)+sizeOfLastBlock;		
	   	//char buffer[BLOCK_SIZE];
	   	char* buffer;
	   	int totalReceived=0,received;			           
	   	unsigned int blockSize = BLOCK_SIZE;         		        
	   	int blockNo = startBlock;
	   	while(totalReceived<totalReqSize){
				buffer = (char*)malloc(BLOCK_SIZE*sizeof(char));
				if(blockNo == lastBlock)
					blockSize = sizeOfLastBlock;
				bzero(buffer, BLOCK_SIZE+1);
				received = recv(this->socket_descriptor, buffer, blockSize, 0);
				if(DEBUG){
	       				cout<<"Received no of nytes: "<<received<<endl;       		
	       				cout<<"Received data: "<<buffer<<endl;       		
				}
				if(received < 0 )
					exit(1);
				totalReceived += received;
				for(int i=0;i<received;i++){
				  dataQ.push(buffer[i]);
				}
				if(DEBUG){
	       	cout<<"Q size: "<<dataQ.size()<<endl;
				}
				if(dataQ.size()>=blockSize){
					for(unsigned int i=0;i<blockSize;i++){
				  	buffer[i] = dataQ.front();
				  	dataQ.pop();
					}
					ofstream file ("."+fileName+to_string(blockNo)+P2P_EXTENSION, ios::out | ios::binary);
					if(DEBUG)
						cout<<"Writing block no: "<<blockNo<<endl;
					if(file.is_open()){
				  	file.write(buffer,blockSize);
				  	file.close();
					}		
					blockNo++;
				}
				free(buffer);       			
			}         	
			close(this->socket_descriptor);
			sourceIPMap[ip]=true;
			mtx.lock();
			downloadThreadNo--;  
			downloaded+=(lastBlock-startBlock+1);       	          
			mtx.unlock();
		}

	  int connectToServer(string remote_ip_address=LOCALHOST, unsigned int remote_port=LISTENINGPORT){
			// Initialise variables
		 	this->remote_ip_address = remote_ip_address;
		 	this->remote_port = remote_port;
			// Initialise struct to hold information about server to which to connect to
		 	struct sockaddr_in server;
		 	memset(&server,0, sizeof(server));
		 	server.sin_family = AF_INET;
		 	server.sin_addr.s_addr=inet_addr(this->remote_ip_address.c_str());
		 	server.sin_port = htons(this->remote_port);
	   	if(DEBUG){
	   		cout<<"Attempting to connect to "<<this->remote_ip_address<<" at port "<<this->remote_port<<endl;
	   		cout<<"Connection going from "<<this->self_ip_address<<" and port "<<this->listening_port<<endl;
	   	}
	   	fflush(stdout);
			// Connect to the server. Error handling to be done.
		 	int status =	connect(this->socket_descriptor, (struct sockaddr*)&server, sizeof(server));
		 	return status;
		}

	  void sendDataToServer(string data){
	   	if(DEBUG)
		 		cout<<"Sending data : "<<data<<endl;
		 	char* buffer = strdup(data.c_str());
		 	this->data = buffer;
		 	int status = send(this->socket_descriptor, this->data, strlen(this->data),0);
		 	if(DEBUG)
		 		cout<<"Send status: "<<status<<endl;
		}

		void getServerDetails(){
			if(DEBUG){
				cout<<"IP Address :: "<<this->self_ip_address;
				cout<<"Port :: "<<this->listening_port;
				cout<<"Number of connections :: "<<this->number_of_connections;
			}
		}

		void getClientDetails(){
			if(DEBUG){
				cout<<"IP Address :: "<<this->self_ip_address;
				cout<<"Port :: "<<this->listening_port;
				cout<<"Server IP Address :: "<<this->remote_ip_address;
				cout<<"Server Port :: "<<this->remote_port;
			}
		}	

		void setReceiveTimeout(time_t seconds){
			// Ref : http://stackoverflow.com/questions/2876024/linux-is-there-a-read-or-recv-from-socket-with-timeout
			struct timeval timeout;
			timeout.tv_sec = seconds;
			timeout.tv_usec = 0; 	
			setsockopt(this->socket_descriptor, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));
		}

		void sendDatagramToServer(char* data, string remote_ip_address=REMOTE, unsigned int remote_port=LISTENINGPORT){
			// Initialise variables
			this->data = data;
			this->remote_ip_address = remote_ip_address;
			this->remote_port = remote_port;
			// Initialise struct to hold information about server to which the datagram is to be sent
			struct sockaddr_in server;
			memset(&server,0, sizeof(server));
			server.sin_family = AF_INET;
			server.sin_addr.s_addr=inet_addr(this->remote_ip_address.c_str());
			server.sin_port = htons(this->remote_port);
			if(DEBUG){
				cout<<"Sending data to : "<<remote_ip_address<<" and port : "<<remote_port<<endl;
				cout<<"Data to be broadcasted : "<<this->data<<endl;
				cout<<"Length of data to be broadcasted : "<<strlen(this->data)<<endl;
				cout<<"ErrNumber before sendto : "<<strerror(errno);
			}
			int res =sendto(this->socket_descriptor, this->data, strlen(this->data),0, (struct sockaddr*)&server, sizeof(server));
			if(DEBUG){
				cout<<"ErrNumber after sendto : "<<strerror(errno)<<endl;
				cout<<"Status : "<<res<<endl;
				cout<<data;
				cout<<strlen(this->data);
			}
			fflush(stdout);
		}

		void broadcast(char* data){
			int broadcast = 1;
			socklen_t optlen;
			// Check if broadcast flag is already set for the socket.
			setsockopt(this->socket_descriptor, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(optlen));
			getsockopt(this->socket_descriptor, SOL_SOCKET, SO_BROADCAST, &broadcast, &optlen);
			if(DEBUG)
				cout<<"Broadcast set : "<<broadcast<<endl;
			this->sendDatagramToServer(data, BROADCAST_IP, BROADCAST_PORT);	
		}

		// Use strtok to convert a string into tokens and return a vector of the tokens
		void split(string s, vector<string>&v, string delimiters){
			char* buffer = new char[s.length()+1];
			strcpy(buffer,s.c_str());
			char* p;
			p = strtok(buffer, delimiters.c_str());
			while(p != NULL){
				v.push_back(p);
				p = strtok(NULL, delimiters.c_str());
			}
		}
};