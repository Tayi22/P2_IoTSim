/*
 * MetaData.h
 *
 *  Created on: Jul 14, 2019
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_METADATA_H_
#define SCRATCH_IOTSIM_METADATA_H_

#include "../Sim_Include.h"
using namespace std;

class MetaData{
private:

	const char* folder = "scratch/IoTSim_Wifi/MetaData/";

	void loadData(string path){
		string line;
		ifstream inputFile;
		bool key = true;
		string key_val;

		inputFile.open(path);

		while(getline(inputFile, line)){
			if (key){
				key_val = line;
				key = false;
			} else {
				data[key_val] = stof(line);
				key = true;
			}
		}

		inputFile.close();

	}
public:
	map<string, float> data;

	MetaData(){}

	MetaData(string path){
		path = folder + path;
		loadData(path);

	}

	float getData(string dataName){
		map<string, float>::iterator it;
		std::string msg;
		if (data.empty()) NS_LOG_UNCOND("Data is empty");

		it = data.find(dataName);

		if (it == data.end()) throw invalid_argument(dataName + " not found in MetaData getData()");

		return it->second;
	}

	void print(){
		std::string print;
		map<string, float>::iterator it;

		for (it = data.begin(); it != data.end(); it ++){
			print += it->first + " " + std::to_string(it->second) + '\n';
		}

		NS_LOG_UNCOND(print);
	}



};



#endif /* SCRATCH_IOTSIM_METADATA_H_ */
