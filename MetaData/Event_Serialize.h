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
	std::ofstream nTaskStream;
	std::ofstream cycleStream;
	std::ofstream infoStream;
	mutex m;



public:
	EventSerialize(string nTaskFile, string cycleFile, string infoFile){
		if (!nTaskStream.is_open()){
			nTaskStream.open(folder + nTaskFile);
			nTaskStream << "Timestamp;NodeId;NodeType;EventType;Event;CompletionTime;CPU;Running_NT;Storage;MAX_Storage;RAM;MAX_RAM\n";
		}

		if (!cycleStream.is_open()){
			cycleStream.open(folder + cycleFile);
			cycleStream << "Timestamp;NodeId;NodeType;EventType;Event;CompletionTime;Tries\n";
		}

		if (!infoStream.is_open()){
			infoStream.open(folder + infoFile);
			infoStream << "Timestamp;NodeId;NodeType;EventType;Event\n";
		}
	}

	EventSerialize(){

	}

	void close(){
		if (nTaskStream.is_open()){
			nTaskStream.flush();
			nTaskStream.close();
		}
		if (cycleStream.is_open()){
			cycleStream.flush();
			cycleStream.close();
		}
		if (infoStream.is_open()){
			infoStream.flush();
			infoStream.close();
		}
	}

	void saveNTaskFinish(int timestamp, string nodeId, string nodeType, string eventType, string event, int completionTime, float cpu, int running_nt, int storage, int storage_max, int ram, int ram_max){
		m.lock();
		nTaskStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << ";" << completionTime << ";" << cpu << ";" << running_nt << ";" << storage << ";" << storage_max << ";" << ram << ";" << ram_max << '\n';
		m.unlock();
	}

	void saveCycleFinish(int timestamp, string nodeId, string nodeType, string eventType, string event, int completionTime, int tries){
		m.lock();
		cycleStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << ";" << completionTime << ";" << tries << '\n';
		m.unlock();
	}

	void saveInfo(int timestamp, string nodeId, string nodeType, string eventType, string event){
		m.lock();
		infoStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << '\n';
		m.unlock();
	}


};



#endif /* SCRATCH_IOTSIM_METADATA_EVENT_SERIALIZE_H_ */
