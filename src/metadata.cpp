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

void MetaData::writeMetaData(string fileName){
	MetaData metaData;
	metaData.fileName = fileName;
	Utils obj;
	pair<long long int, long long int> pll;
	pll = obj.getSizeAndNoOfBlocks(fileName);
	metaData.fileSize=pll.first;
	metaData.noOfBlocks=pll.second;
	metaData.fileHash= obj.getMD5OfFile(fileName);
	metaData.lastUpdated=time(0);
	string metaFileName = fileName+ META_EXTENSION+P2P_EXTENSION;
	ofstream out(metaFileName.c_str());
	out<<metaData;
	out.close();
}

MetaData MetaData::readMetaData(string fileName){
  MetaData metaData;
  string metaFileName = fileName+ META_EXTENSION+P2P_EXTENSION;
  ifstream ifs(metaFileName.c_str());
	return metaData;
}

bool MetaData::verifyHash(string filename){
	MetaData metaData = readMetaData(filename);
	string hash = metaData.fileHash;
	Utils obj;
	string downloadedFileHash = obj.getMD5OfFile(filename);
	if(obj.equality(hash, downloadedFileHash))
		return true;
	return false;
}