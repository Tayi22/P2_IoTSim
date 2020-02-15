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
#include <math.h>

// From https://github.com/nlohmann/json
using json = nlohmann::json;


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
	float single_Data_access;

	void init(std::string jsonFile, float sda){
		single_Data_access = sda;
		std::ifstream i(folder + jsonFile);
		i >> jo;
		srand (time(NULL));
	}

	int getCondition(std::string c, std::string s, std::string& con){
		try{
			con = jo[c][s]["condition"];
		} catch (...){
			return -1;
		}
		return 0;
	}


	int getNextStep(std::string c, std::string s, int option, int& nextStep, std::string& sendTo){
		json next_step;
		try{
			next_step = jo[c][s]["next_step"];
		} catch (...){
			nextStep = -1;
			sendTo = "";
			return 0;
		}

		std::string opt = "Option_" + std::to_string(option);
		nextStep = next_step[opt]["nextStep"];
		sendTo = next_step[opt]["sendTo"];

		return 0;
	}

	int getDeadPayloadSize(std::string c, std::string s, int num_revoke_cert){
		json payload;
		int payloadSize = 1;
		try{
			payload = jo[c][s]["payload"];
			payloadSize = payload["size"];
			std::string mult = payload["mult_with"];
			if (mult.compare("num_revo_cert") == 0) payloadSize *= num_revoke_cert;
		} catch (...){
			return -1;
		}

		return payloadSize;
	}

	int getStorageData(std::string c, std::string s, int& crl, int& key, int& cert){
		json storage;
		try{
			storage = jo[c][s]["storage"];
		} catch (...){
			return -1;
		}

		try{
			crl += storage["crl"];
			if (crl < 0) crl = 0;
			key += storage["key"];
			if (key < 0) key = 0;
			cert += storage["cert"];
			if (cert < 0) cert = 0;
		} catch (...){
			throw "Storage was defined but crl, key or cert is missing";
		}
		return 0;
	}

	int getTimeRam(std::string c, std::string s, float& time, int& ram){
		json data = jo[c][s];
		time = data["time"];
		ram = data["ram_strain"];
		return 0;
	}

	int getAccTime(std::string c, std::string s, float& accTime, int num_revo){
		std::string type;
		try{
			type = jo[c][s]["check_revoke_list"];
			if (type.compare("OSCP") == 0){
				accTime = single_data_access * log10(num_revo);
			} else if (type.compare("CRL") == 0){
				accTime = single_data_access * num_revo;
			}
		} catch (...){
			return -1;
		}
		return 0;
	}

	int checkvalid(std::string c, std::string s, bool& shallCheck){
		try{
			shallCheck = jo[c][s]["check_valid_aff"];
		} catch (const char* msg){
			throw(msg);
		} catch (...){
			return -1;
		}
		return 0;
	}

	int getAddCert(std::string c, std::string s){
		try{
			return jo[c][s]["add_cert"];
		} catch (...){
			return 0;
		}
	}

	float dataAccTimeAdd(std::string c, std::string s, int n){
		try{
			std::string temp = jo[c][s]["O()"];
			switch(temp[0]) {
			case 'l':
				return log10(n) * single_Data_access;
				break;
			case 'n':
				return single_Data_access * n;
				break;
			default:
				return 0.f;
			}

		} catch (...){
			return 0.f;
		}
	}

	int getCrlStorage(std::string c, std::string s, int n){
		int crl_storage;
		try{
			crl_storage = jo[c][s]["crl_storage"];
			crl_storage *= n;
		} catch (...){
			crl_storage = 0;
		}
		return crl_storage;
	}


	void getStorageData(std::string c, std::string s, int &needStorage, int &needRam){
		jm.lock();
		json storage = jo[c][s];
		needStorage = storage["storage_strain"];
		needRam = storage["ram_strain"];
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
