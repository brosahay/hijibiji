 /* JSON File : https://github.com/nlohmann/json */
#include <iostream>
#include <map>
#include <string>
#include <dirent.h>
#include <chrono>
#include <typeinfo>

#include "utils.h"
#include "constants.h"
#include "metadata.h"
#include "connectionManager.cpp"
#include "json.hpp"

#define BROADCAST_IP "255.255.255.255"
#define BROADCAST_PORT 9091

using json = nlohmann::json;
using namespace std;

struct FileDetailsList{
	string filename;
	long long int size;
	vector<string> source;
};

// localFileList : Merged with remoteFileList
// remoteFileList : FileList received from the source
// selfFileList : FileList to be broadcasted

//GLOBAL VAR to maintain local FileList
map<string, FileDetailsList> localFileList;

class FileList
{
public:
	void printMap(std::map<string,FileDetailsList> m) {
		FileDetailsList tmp;
		string filename;
		long long int size;
		string ip,ip2;
		if(DEBUG)
		{
			cout<<"Map size : "<<m.size()<<endl;
			cout<<"Printing File Map : "<<endl; 
		}
		for(std::map<string,FileDetailsList>::iterator it=m.begin();it!=m.end();++it){
			tmp = it->second;
			filename = tmp.filename;
			size = tmp.size;
			if(DEBUG)
			{
				cout<<"Number of sources : "<<tmp.source.size()<<endl;
				cout << it->first<<":"<<filename<<":"<<size<<":";
				for(vector<string>::iterator it=tmp.source.begin();it!=tmp.source.end();++it){
					cout<<*it<<":";
				}
				cout<<endl;
			}
		}
	}


	// Parse json from source and returns the corresponding map
	map<string, FileDetailsList> loadFileListString(const string* jsonString,const string* sourceIP){

		json sourceJson = json::parse(*jsonString);
		string hash;
		FileDetailsList item;
		map<string, FileDetailsList> sourceFileList;

		for (json::iterator it = sourceJson.begin(); it != sourceJson.end(); ++it) {
			hash=it.key();
			// Clear the IP source vector before appending data
			item.source.clear();
			item.filename=sourceJson[hash]["filename"];
			item.size=sourceJson[hash]["size"];
			item.source.push_back(*sourceIP);
			sourceFileList[hash]=item;
			if(DEBUG){
				cout<<"Hash :"<<hash<<endl;
				cout<<"Filename : "<<item.filename<<endl;
			}
		}

		return sourceFileList;
	}

	// Reads in a map of file list and returns a string in json format to be sent over the network
	string dumpFileListString(const map <string,FileDetailsList> *fileList){

		json j;

		string hash, filename;
		long long int size;
		FileDetailsList tmp;
		

		for(std::map<string,FileDetailsList>::const_iterator it=fileList->begin();it!=fileList->end();++it){
			hash = it->first;
			tmp = it->second;

			filename = tmp.filename;
			size = tmp.size;

			j[hash] = { {"filename" , filename}, {"size", size} };

		}
		return j.dump();
	}

	// Return true if i and j are equal
	bool equality(string i, string j){
		if (DEBUG)
		{
			cout<<"Testing for "<<i<<" and "<<j<<endl;
			cout<<"Result : "<<i.compare(j)<<endl;
		}
		if(!i.compare(j))
			return true;
		return false;
	//	return (!i.compare(j));
	}

	// Generate self file list
	map<string, FileDetailsList> prepareSelfFileList(){
		
		Utils obj;
		string filename, hash;
		pair<long long int, long long int> sizeAndBlock;
		long long int size;
		FileDetailsList item;
		vector<string> source;
		
		map<string, FileDetailsList> selfFileList;
		// Prepare list of files
		vector<string> fileList = obj.getFilesInDirectory();
		if(DEBUG)		
		{
			//cout<<"Number of files in directory : "<<fileList.size()<<endl;
		}
		// For each file, populate the map with hash, filename and size
		for(int i=0; i<fileList.size(); i++){
			filename = fileList[i];
			hash = obj.getMD5OfFile(filename);
			sizeAndBlock = obj.getSizeAndNoOfBlocks(filename);
			size = sizeAndBlock.first;
			// For testing purpose : Pass ip as string args and uncomment lines below, modify item structure.
			// source.clear();
			// source.push_back(ip);
			
			item = {filename, size};
			selfFileList[hash] = item;
			if(DEBUG){
				cout<<"Filename : "<<filename<<endl;
				cout<<"Size : "<<size<<endl;
				cout<<"MD5 : "<<hash<<endl;
			}
			MetaData metadataObj;
			metadataObj.writeMetaData(filename);
		}

		return selfFileList;
	}

	//Merge FileList maps
	void mergeFileList(map<string, FileDetailsList> *remoteFileList){
		string hash;	
		FileDetailsList tmp;
		int newFiles = 0;
		map<string, FileDetailsList> selfFileList = prepareSelfFileList();
		if(DEBUG)
		{
			cout<<"Attempting to merge file lists"<<endl;
		}
		for(map<string,FileDetailsList>::iterator it=remoteFileList->begin();it!=remoteFileList->end();++it){
			hash = it->first;
			// If count of hash is greater than 0, then hash exists in selfFileList. So the file is present in local machine.
			if(selfFileList.count(hash) > 0)
				continue;
			// If count of hash is greater than 0, then hash entry exists in localFileList
			if(localFileList.count(hash) > 0){
				tmp = it->second;
				if(DEBUG){
					cout<<"Hash previously exists. Attempting to merge source."<<endl;
					cout<<"Source of File List : "<<tmp.source[0]<<endl;
				}
				
				localFileList[hash].source.push_back(tmp.source[0]);
				// Sort and keep unique IPs only.	
				sort(localFileList[hash].source.begin(), localFileList[hash].source.end());
				localFileList[hash].source.erase(unique(localFileList[hash].source.begin(), localFileList[hash].source.end()), localFileList[hash].source.end());
			}
			else{
				newFiles++;
				if(DEBUG){
					cout<<"New file hash found. Hash :"<<hash<<endl;
				}
				tmp = {it->second.filename, it->second.size, it->second.source};
				localFileList[hash]=tmp;
			}
		}
		if(newFiles>0)
		{
			//cout<<"\r";
			//cout<<"Enter index of the file to be downloaded (0 to cancel): ";
			//cout<<endl;
			//cout<<"New files have been added to list.\n"<<endl;			
		}				
	}


	// Broadcast the FileList
	void broadcastFileList(){
		map<string, FileDetailsList> selfFileList = prepareSelfFileList();
		string data = dumpFileListString(&selfFileList);
		char *dataToSend = strdup(data.c_str());
		//UDP for file transfer
		ConnectionManager sender(false); 
		//Broadcast 
		for(;;){
			sender.broadcast(dataToSend);
			// Send the thread to sleep for 60s
			this_thread::sleep_for(std::chrono::seconds(BROADCAST_INTERVAL));
		}

		/* TODO : FREE MAP HERE. REQUIRED? */
	}

	//Listen for broadcasting FileList
	void startListeningBroadcast(){
	  //UDP socket
		ConnectionManager listner(false, BROADCAST_PORT);
	  //Bind port and listen
		listner.startServer();
		for(;;)
		{
	    //catch broadcasting packet
			pair <string,string> sourceDatagram;
			sourceDatagram= listner.readDatagram();
			string jsonString;
	  	//returns Corresponding Broadcast IP
			string sourceIP = string(sourceDatagram.first);
	    //returns DataString
			jsonString = sourceDatagram.second;		
			map<string, FileDetailsList> remoteFileList=loadFileListString(&jsonString,&sourceIP);
			mergeFileList(&remoteFileList);
			if(DEBUG){
				cout<<"Broadcast Source IP :"<<sourceIP;
			}
	    //relay to be processed

		}
	}
};
/*int main(){
	char ch;
	cin>>ch;
	switch(ch){
		case 's':
		broadcastFileList();
		break;
		case 'r':
		startListeningBroadcast();
		break;
	}
	return 0;
}*/
