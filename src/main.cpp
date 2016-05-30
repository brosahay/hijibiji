#include <bits/stdc++.h>
#include <execinfo.h>
#include "utils.h"
#include "metadata.h"

#include "constants.h"
#include "files.cpp"
#include "metadata.h"

bool print;

//#include "connectionManager.cpp"
using namespace std;

extern map<string, FileDetailsList> localFileList;
extern map<string,bool> sourceIPMap;

extern mutex mtx;
vector<thread> downloadThreadVector;
unsigned int downloadThreadNo;
long long int downloaded;
vector<string> hashes;


void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


void merge(string fileName, int noOfBlocks, int sizeOfLastBlock)
{
    cout<<"Merging files...\r";
    int blockSize = BLOCK_SIZE;
    char buffer[BLOCK_SIZE];
    ofstream writer(fileName, ios::out|ios::binary);
    for(int i=0;i<noOfBlocks;i++)
    {
        ifstream reader("."+fileName+to_string(i)+P2P_EXTENSION, ios::in | ios::binary);
        bzero(buffer, BLOCK_SIZE+1);
        if(i==noOfBlocks-1)
            blockSize=sizeOfLastBlock;
        reader.read(buffer,blockSize);
        writer.write(buffer,blockSize);
        reader.close();
        remove(("."+fileName+to_string(i)+P2P_EXTENSION).c_str());
    }
    writer.close();
    cout<<"Merging completed."<<endl;    
}

void writeMetaToFile(string metaFileName, string response)
{
    ofstream out(metaFileName.c_str(),ios::binary);
    //cout<<response.c_str();
    out.write(response.c_str(),response.length());
    //out<<response;
    out.close();  
}

void startListeningOnMetaPort()
{
    ConnectionManager receiver(true, 9080);
    receiver.startServer();
    for(;;)
        receiver.acceptConnection();
}

void startListeningOnBlocksPort()
{
    ConnectionManager receiver(true, BLOCKS_REQUEST_PORT);
    receiver.startServer();    
    receiver.acceptConnection();
}

void downloadBlocks(string ip, string fileName, int startBlock, int lastBlock, int sizeOfLastBlock)//(string ip, string fileName, int startBlock, int lastBlock, int sizeOfLastBlock)
{
    ConnectionManager fetchBlocksTCP(true, BLOCKS_DATA_PORT);
    //fetchBlocksTCP.setReceiveTimeout((time_t)30);    
    fetchBlocksTCP.fetchBlocks(ip,fileName,startBlock,lastBlock,sizeOfLastBlock);                    
}

void displayFileList()
{
    hashes.clear();
    int indx=1;
    cout<<"Files available for download: "<<endl;
    for(map<string,FileDetailsList>::iterator it=localFileList.begin();it!=localFileList.end();it++,indx++)
    {   
        cout<<indx<<". "<<it->second.filename<<endl;
        hashes.push_back(it->first);
    }
}

thread primeThreads[4];

int main()
{

    signal(SIGSEGV, handler);
    print=false;

    //1. start listening for file list broadcast
    //primeThreads[0] = thread(startListeningBroadcast);
    FileList fileListObj1;
    primeThreads[0] = thread(&FileList::startListeningBroadcast,&fileListObj1);

    //2. start listening for meta file requests
    primeThreads[1] = thread(startListeningOnMetaPort);

    // 3. start listening for file blocks requests
    primeThreads[2] = thread(startListeningOnBlocksPort);    

    //4. update and broadcast file list
    //broadcastListThread = thread(broadcastFileList);
    FileList fileListObj2;
    primeThreads[3] = thread(&FileList::broadcastFileList,&fileListObj2);

    bool listShownOnce=false;    
    string command;    
    cout<<"Waiting for file list from other online users..."<<endl;
    do
    {
        if(listShownOnce==false)
        {
            if(localFileList.size()>0)
            {
                //display file list
                displayFileList();
                listShownOnce=true;
            }            
        }        
        if(listShownOnce==true)
        {
            cout<<"\nEnter command (list/download/exit): ";
            cin>>command;
            if(command=="download")
            {            
                int reqdIndex;
                downloadThreadVector.clear();
                downloadThreadNo=0;
                sourceIPMap.clear();
                string fileName;            
                map<string,FileDetailsList>::iterator localFileListIterator;
                //1.get file name
                cout<<"Enter index of the file to be downloaded (0 to cancel): ";
                cin>>reqdIndex;                
                while(reqdIndex<0 || reqdIndex>hashes.size())
                {
                    cout<<"Invalid index entered."<<endl;
                    cout<<"Enter index of the file to be downloaded (0 to cancel): ";
                    cin>>reqdIndex;           
                }
                if(reqdIndex==0)
                    continue;

                fileName=localFileList[hashes[reqdIndex-1]].filename;                
                cout<<"Downloading "<<fileName<<":"<<endl;

                for(int i=0;i<localFileList[hashes[reqdIndex-1]].source.size();i++)
                {
                    sourceIPMap[localFileList[hashes[reqdIndex-1]].source[i]]=true;
                    if(DEBUG)
                        cout<<localFileList[hashes[reqdIndex-1]].source[i]<<endl;
                }

                //2.process merged list to get ip of all pc having the desired file            
                //sourceIPMap["127.0.0.1"]=true;            
                //sourceIPMap["192.168.0.121"]=true;  
                //sourceIPMap["192.168.0.103"]=true;  
                //sourceIPMap["192.168.1.148"]=true;            
                //3.get meta from any one
                // Question : Map is created with hash as key. Would it make more sense to use hash for requests and stuff?
                //yes, will do that later            
                char* dataToSend = strdup(fileName.c_str());

                // Question : Do I need to send out requests to all IPs? Maybe introduce a timeout?
                //even i thought for timeout. actually, i will break the loop if meta is received from one pc
                // Create listener object just once
                ConnectionManager senderMetaTCP(true, META_FILE_PORT);

                // Set a receive timeout for 30s. This removes the blocking nature of the socket.
                // Using future::wait_for to get meta
                // Need help in implementation 

                // Ref : http://en.cppreference.com/w/cpp/thread/future/wait_for
                // Ref : http://www.cplusplus.com/reference/future/future/wait_for/

                //senderMetaTCP.setReceiveTimeout((time_t)30);
                string response, metaFileName;
                Utils utilsobj;
                printf("0%% downloaded\r");
                // Currently hooked to echo server.
                bool metaDownloaded = false;
                for(int i=0; i<localFileList[hashes[reqdIndex-1]].source.size(), metaDownloaded==false;i++){
                    int statusConnect = senderMetaTCP.connectToServer(localFileList[hashes[reqdIndex-1]].source[0],META_REQUEST_PORT);
                    if(DEBUG)
                    {
                        cout<<"status:"<<statusConnect<<endl;
                    }
                    if(statusConnect==-1)
                        continue;
                    senderMetaTCP.sendDataToServer(dataToSend);
                    response = senderMetaTCP.readData();
                    if(DEBUG)
                        cout<<"Response received : "<<response;
                   /* if(utilsobj.equality(response, "META -1")){
                        cout<<"The file is not yet ready for download."<<endl;
                        // Remove file whose meta is not yet ready.
                         localFileListIterator = localFileList.find(hashes[reqdIndex-1]);
                        localFileList.erase(localFileListIterator);
                        continue;
                    }*/

                    // If meta is present, then write to file and set metaDownloaded to true
                    if(!utilsobj.equality(response, "META -1")){
                        if(DEBUG){
                            cout<<"Metadata received. Writing to file."<<endl;
                        }
                        metaDownloaded = true;
                        metaFileName = fileName+META_EXTENSION+P2P_EXTENSION;
                        writeMetaToFile(metaFileName,response);                        
                        
                    }
                }

                // If meta hasnt been received, alert user.
                if(!metaDownloaded){
                    cout<<"The file is not yet ready for download"<<endl;
                    continue;
                }
                 


                //4.repeat 3 until meta is received                
                
                //5.after receiving meta, decide divison of blocks over IPs
                MetaData metadataObj;
                MetaData metaData = metadataObj.readMetaData(fileName);
                long long int noOfBlocks = metaData.noOfBlocks;
                int sizeOfLastBlock = BLOCK_SIZE;
                long long int blocksLeft = noOfBlocks;
                int noOfSources = sourceIPMap.size();
                int blocksPerSource = mini(noOfBlocks,BLOCKS_PER_SOURCE);            
                int startBlock = 0;
                int lastBlock = mini(blocksPerSource-1, blocksLeft-1);      

                //6.send specific request in separate threads
                if(DEBUG)
                    cout<<"download info: "<<startBlock<<" "<<lastBlock<<" "<<sizeOfLastBlock<<" "<<blocksPerSource<<" "<<noOfBlocks<<endl;
                
                downloaded=0;
                bool completed=false;            
                do
                {
                    for(map<string,bool>::iterator it = sourceIPMap.begin();it!=sourceIPMap.end();it++)
                    {
                        if(it->second==true)
                        {   
                            if(DEBUG)                    
                                cout<<startBlock<<" "<<lastBlock<<endl;
                            string ip = it->first;
                            it->second=false;
                            if(lastBlock==noOfBlocks-1)
                                sizeOfLastBlock=metaData.fileSize - ((noOfBlocks-1)*BLOCK_SIZE);
                            downloadThreadVector.push_back(thread(downloadBlocks,ip,fileName,startBlock,lastBlock,sizeOfLastBlock));
                            mtx.lock();
                            downloadThreadNo++;
                            mtx.unlock();
                            if(lastBlock==noOfBlocks-1)
                                completed=true;
                            startBlock=lastBlock+1;
                            lastBlock = mini((startBlock+BLOCKS_PER_SOURCE-1),(noOfBlocks-1));                                 
                            //break;
                            if(completed==true)
                                break;
                        }
                    }
                    for (auto& th : downloadThreadVector)
                    {
                        if(th.joinable())
                            th.join();
                    }
                    downloadThreadVector.clear();
                    for(int i=0;i<localFileList[hashes[reqdIndex-1]].source.size();i++)
                    {
                        sourceIPMap[localFileList[hashes[reqdIndex-1]].source[i]]=true;
                        if(DEBUG)
                            cout<<localFileList[hashes[reqdIndex-1]].source[i]<<endl;
                    }
                    double percentage = ((double)(downloaded)/(noOfBlocks))*100;
                    if(percentage<100)
                        printf("%.0lf%% downloaded\r",((double)(downloaded)/(noOfBlocks))*100);
                    else
                        printf("%.0lf%% downloaded\n",((double)(downloaded)/(noOfBlocks))*100);
                }while(completed==false);
                merge(fileName,noOfBlocks,sizeOfLastBlock);

                // Remove the downloaded hash from the localFileList
                if(DEBUG){
                    cout<<"Deleting file : "<<localFileList[hashes[reqdIndex-1]].filename<<" from list."<<endl;
                }
                localFileListIterator = localFileList.find(hashes[reqdIndex-1]);
                localFileList.erase(localFileListIterator);

                //7.repeat 5 and 6 untill all blocks all downloaded 
            }
            else if(command == "list")
            {
                if(localFileList.size()>0)
                    displayFileList();
                else
                    cout<<"Currently no new file is present on the network. Try again."<<endl;
            }    
            else if(command=="exit")
            {
                for(int i=0; i<4; i++){
                    if(DEBUG){
                        cout<<"Thread TID : "<<primeThreads[i].get_id()<<endl;
                    }
                    primeThreads[i].detach();
                }
            }                             
            else
            {
                cout<<"Invalid command!\n";
            }  
        }              
    }while(command!="exit");
	return 0;
}
    
