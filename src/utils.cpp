#include <iostream>
#include <dirent.h>
#include <openssl/md5.h>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"
#include "metadata.h"
#include "constants.h"

using namespace std;

vector<string> Utils::getFilesInDirectory(){
	DIR *dpdf;
	class dirent *epdf;
	vector<string> fileNames;
	dpdf = opendir("./");
	if (dpdf != NULL){
		while (epdf = readdir(dpdf)){
			if(isRegularFile(epdf->d_name) == true && epdf->d_name[0]!='.'){
				string extension = (string)(epdf->d_name);
				if(extension.length()>=4){
					extension = extension.substr(extension.length()-4,4);	
					if(extension==P2P_EXTENSION)
						continue;					
				}
				fileNames.push_back(epdf->d_name);   				
			}   				
		}
	}
	closedir(dpdf);
	return fileNames;
}

bool Utils::isRegularFile(const char *path){
	struct stat path_stat;
	stat(path, &path_stat);
	return (bool)S_ISREG(path_stat.st_mode);
}

pair<long long int,long long int> Utils::getSizeAndNoOfBlocks(string fileName){
	pair<long long int, long long int> pll;
	long long int parts,fileSize;
	ifstream infile(fileName.c_str(), ios::binary | ios::ate);
	if (infile.is_open()){
		fileSize=infile.tellg();
		parts=fileSize/BUFFER_SIZE;
		if(BUFFER_SIZE*parts<fileSize)
			parts++;
		infile.close();
	}
	pll = make_pair(fileSize,parts);
	return pll;
}

string Utils::getMD5(string text){
	string md5;
	unsigned char result[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)text.c_str(), text.size(), result);
	char buf[MD5_DIGEST_LENGTH];
	for (int i=0;i<MD5_DIGEST_LENGTH;i++){
		sprintf(buf, "%02x", result[i]);
		md5.append( buf );
	}
	return md5;
}

/*Usage : md5(<filename>)*/
string Utils::getMD5OfFile(string filename){
	unsigned char hash[MD5_DIGEST_LENGTH];
	MD5_CTX md5Context;
	int bytes;
	// Buffer to read data from file
	char data[BLOCK_SIZE];
	// Buffer to hold data while conversion of md5 to string
	char buff[2];
	//Final string
	string md5;
	//Assuming file exists. Error checking to be done
	FILE *in = fopen(filename.c_str(), "rb");
	MD5_Init(&md5Context);
	while((bytes = fread(data, 1, BLOCK_SIZE, in))!=0){
		if(DEBUG){
			cout<<"Bytes read :: "<<bytes<<endl;
		}
		MD5_Update(&md5Context, data, bytes);
	}
	MD5_Final(hash,&md5Context);
	for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
		sprintf(buff,"%02x",hash[i]);
		md5.append(buff);
	}
	if(DEBUG){
		cout<<filename<<": "<<md5<<endl;
	}
	fclose(in);
	return md5;
}

void Utils::updateFilesList(){
	vector<string> fileNames;
	fileNames = getFilesInDirectory();
}

bool Utils::equality(string i, string j){
	if(DEBUG){
		cout<<"Testing for "<<i<<" and "<<j<<endl;
		cout<<"Result : "<<i.compare(j)<<endl;
	}
	if(!i.compare(j))
		return true;
	return false;
}