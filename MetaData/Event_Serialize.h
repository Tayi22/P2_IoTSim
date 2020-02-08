/*
 * Event_Serialize.h
 *
 *  Created on: Jul 21, 2019
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_METADATA_EVENT_SERIALIZE_H_
#define SCRATCH_IOTSIM_METADATA_EVENT_SERIALIZE_H_

#include "../Sim_Include.h"

using namespace std;

class EventSerialize{
private:
	const char* folder = "scratch/IoTSim_Wifi/MetaData/Output/";
	std::ofstream outputStream;
	mutex m;



public:
	EventSerialize(string fileName){
		if (!outputStream.is_open()){
			outputStream.open(folder + fileName);
			outputStream << "Timestamp;NodeId;NodeType;EventType;Event;CompletionTime;CPU;Running_NT;Storage;MAX_Storage;RAM;MAX_RAM\n";
		}
	}

	EventSerialize(){

	}

	void close(){
		if (outputStream.is_open()){
			outputStream.flush();
			outputStream.close();
		}
	}


	void write(int timestamp, string nodeId, NodeType nodeType, EventType eventType, string event, int completionTime = 0, float cpu = 0.f, int running_NT = 0, int storage = 0, int ram = 0, int maxStorage = 0, int maxRam = 0){
		m.lock();
		outputStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << ";" << completionTime << ";" << cpu << ";" << running_NT << ";" << storage << ";" << maxStorage << ";" << ram << ";" << maxRam << '\n';
		m.unlock();
	}


};



#endif /* SCRATCH_IOTSIM_METADATA_EVENT_SERIALIZE_H_ */
