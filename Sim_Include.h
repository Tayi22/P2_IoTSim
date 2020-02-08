/*
 * Sim_Include.h
 *
 *  Created on: Jul 14, 2019
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_SIM_INCLUDE_H_
#define SCRATCH_IOTSIM_SIM_INCLUDE_H_

enum NodeType {R_NODE, L_NODE, I_NODE};
enum EventType {INFO, CREATED, VALIDATED, ERROR, NTASK_FINISH, WTASK_FINISH, EXPIRED, REVOKED, PACKET_TRAVEL};
enum TaskType{W_TASK, N_TASK};

#include <iostream>
#include <random>
#include <string>
#include <map>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <ctime>
#include <sstream>
#include <memory>
#include <type_traits>
#include <mutex>
#include <chrono>
#include <thread>
#include "MetaData/JSON/json.hpp"
#include <map>
#include <list>
#include <csignal>

// From https://github.com/nlohmann/json
using json = nlohmann::json;


std::string getNodeTypeString(NodeType nt){
	switch (nt){
	case R_NODE:
		return "R_NODE";
	case L_NODE:
		return "L_NODE";
	case I_NODE:
		return "I_NODE";
	default:
		return "NO_NODE";
	}
}

std::string getThisThreadId(){
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}



std::string getJsonErrString(std::string eWhat, std::string cycle, std::string step){
	std::string errString(eWhat);
	errString = "Could not read Json: " + errString + " Cycle: " + cycle + " NextStep_: " + step;
	return errString;
}


int wt_retry_time;
int status_unix_intervall;


struct JsonRead{
	json jo;
	const char* folder = "scratch/IoTSim_Wifi/MetaData/";
	std::mutex jm;

	void init(std::string jsonFile){
		std::ifstream i(folder + jsonFile);
		i >> jo;
		srand (time(NULL));
	}

	// Returns the String used to find the actual multiplier in the MetaData of the Node
	// If "0" then no general Multiplier is used

	std::string getMultiplierString(std::string c, std::string s){
		return jo[c][s]["multiplier"];
	}

	int getCondition(std::string c, std::string s, std::string &con){
		try{
			con = jo[c][s]["condition"];
		} catch (char const* msg){
			con = "";
			return 1;
		}
		return 0;
	}

	float getTime(std::string c, std::string s, float multiplier = 1.0f){
		return (float)(jo[c][s]["time"]) * multiplier;
	}

	void getNextStepCondition(std::string c, std::string s, int &retNextStep, std::string &retSendTo, bool con){
		json options = jo[c][s]["step_on_succ"];
		int index;
		if (con){
			index = 0;
		} else {
			index = 1;
		}
		retNextStep = options[index]["nextStep"];
		retSendTo = options[index]["sendTo"];
		return;
	}

	void getNextStep(std::string c, std::string s, int &retNextStep, std::string &retSendTo){
		// Get all aviable options from the step_on_succ Object.
		json options = jo[c][s]["step_on_succ"];
		int r = rand() % 100 + 1;
		int chance = 0;
		json option;

		if (options.begin() == options.end()){
			retNextStep = -1;
			retSendTo = "o";
			return;
		}

		for (auto it = options.begin(); it != options.end(); it++){
			// Set to a single option
			option = *it;
			// Extract the Chance threshold
			chance = option["chance"];

			// Check if r is lower then threshold, if so, return that step
			if (r <= chance){
				retNextStep = option["nextStep"];
				retSendTo = option["sendTo"];
				return;
			}
		}
		// This should not happen
		throw "No Next step found. Check Json";
	}

	int getDeadPayloadSize(std::string c, std::string s){
		return jo[c][s]["dead_payload_bit"];
	}

	std::string getStorageStrainString(std::string c, std::string s){
		json storage = jo[c][s]["storage"];
		return storage["storage_strain"];
	}

	void getStorageData(std::string c, std::string s, int &needStorage, int &needRam){
		jm.lock();
		json storage = jo[c][s]["storage"];
		needStorage = storage["storage_strain_kB"];
		needRam = storage["ram_strain_kB"];
		jm.unlock();
	}

	char getErrorStep(std::string c, std::string s){
		return (int)(jo[c][s]["step_on_err"]);
	}


};




#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE ("StatusInfo");

#include "MetaData/Event_Serialize.h"
#include "MetaData/MetaData.h"
#include "Payload.h"
#include "WTask.h"
#include "NTask.h"
#include "ANode.h"






#endif /* SCRATCH_IOTSIM_SIM_INCLUDE_H_ */
