#ifndef __CONSTANTS_H_INCLUDED__
#define __CONSTANTS_H_INCLUDED__

#define BUFFER_SIZE 1048576
#define BLOCK_SIZE 1048576
#define BROADCAST_IP "255.255.255.255" //replace this with subnet BROADCAST_IP ref:http://stackoverflow.com/questions/11215605/finding-subnet-mask-on-linux-in-c
#define BROADCAST_PORT 9091
#define BROADCAST_INTERVAL 30

#define META_REQUEST_PORT 9080
#define META_FILE_PORT 9081
#define BLOCKS_REQUEST_PORT 9070
#define BLOCKS_DATA_PORT 9071

#define LOCALHOST "0.0.0.0"
#define LISTENINGPORT 9090
#define DEBUG false
#define NO_OF_THREADS 10
#define min(a,b) a<b ? a : b
#define BROADCAST_IP "255.255.255.255"
#define BROADCAST_PORT 9091
#define REMOTE "127.0.0.1"

#define NO_OF_META_THREADS 10

#define BLOCKS_PER_SOURCE 10

#define mini(x,y) x<y?x:y

const string META_EXTENSION = ".meta";
const string P2P_EXTENSION = ".prv";

#endif
