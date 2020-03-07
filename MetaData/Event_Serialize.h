/*
 * Event_Serialize.h
 *
 *  Created on: Jul 21, 2019
 *      Author: richard
 */

// Lifecycle, NodeId, NodeType,
// Lifecycle, Cycle_Id, StepName, Step

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
	string simName;



public:
	EventSerialize(string nTaskFile, string cycleFile, string infoFile,string simName){
		this->simName = simName;
		if (!nTaskStream.is_open()){
			nTaskStream.open(folder + simName + "_" + nTaskFile);
			nTaskStream << "Timestamp;NodeId;NodeType;EventType;Event;wt_id;CompletionTime;CPU;Running_NT;RAM;MAX_RAM;SimName;StepNum;StepName;StartNodeType;\n";
		}

		if (!cycleStream.is_open()){
			cycleStream.open(folder + simName + "_" + cycleFile);
			cycleStream << "Timestamp;NodeId;NodeType;EventType;Event;wt_id;CompletionTime;Tries;Storage;MAX_Storage;SimName;\n";
		}

		// void save_info(string lifecycle, string cycle = "", string step_name = "", int step_num, int node_id, int node_type){
		if (!infoStream.is_open()){
			infoStream.open(folder + simName + "_" + infoFile);
			infoStream << "lifecycle;cycle;stepName;stepNum;nodeId;nodeType\n";
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

	void saveNTaskFinish(int timestamp, string nodeId, string nodeType, string eventType, string event, int completionTime, float cpu, int running_nt, int ram, int ram_max, int wt_id, int step_num, std::string step_name, std::string startNodeType){
		m.lock();
		nTaskStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << ";" << wt_id << ";" << completionTime << ";" << cpu << ";" << running_nt << ";" << ram << ";" << ram_max << ";" << simName << ";" << step_num << ";" << step_name << ";" << startNodeType << '\n';
		m.unlock();
	}

	void saveCycleFinish(int timestamp, string nodeId, string nodeType, string eventType, string event, int completionTime, int tries, int storage, int storage_max, int wt_id){
		m.lock();
		cycleStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << ";" << wt_id << ";" << completionTime << ";" << tries << ";" << storage << ";" << storage_max << ";" << simName << '\n';
		m.unlock();
	}
	/*
	void saveInfo (int timestamp, string nodeId, string nodeType, string eventType, string event, int wt_id){
		m.lock();
		infoStream << timestamp << ";" << nodeId << ";" << nodeType << ";" << eventType << ";" << event << ";" << wt_id << ";" << simName <<'\n';
		m.unlock();
	}*/

	void saveInfo(string lifecycle, string cycle = "", string step_name = "", int step_num = 0, int node_id = 0, std::string node_type = ""){
		m.lock();
		infoStream << lifecycle << ";" << cycle << ";" << step_name << ";" << step_num << ";" << node_id << ";" << node_type << '\n';
		m.unlock();
	}


};



#endif /* SCRATCH_IOTSIM_METADATA_EVENT_SERIALIZE_H_ */
