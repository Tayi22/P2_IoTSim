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
#include "MetaData/Event_Serialize.h"

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

struct StorageData{
	int key = 0;
	int cert = 0;

	int sym_key_size = 0;
	int asym_key_size = 0;
	int cert_size = 0;
	int crl_entry_size = 0;

	int getStorage(int num_sym_keys, int num_rev_cert){
		return key + (num_sym_keys * sym_key_size) + cert + (crl_entry_size * num_rev_cert);
	}


};


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

	StorageData getConstData(){
		StorageData d;
		json j = jo["const_data"];
		d.sym_key_size = j["sym_key_size"];
		d.cert_size = j["cert_size"];
		d.asym_key_size = j["asym_key_size"];
		d.crl_entry_size = j["crl_entry_size"];
		return d;
	}


	int getNextStep(std::string c, std::string s, int option, int& nextStep, std::string& sendTo){
		json next_step;
		try{
			next_step = jo[c][s]["next_step"];
			std::string opt = "Option_" + std::to_string(option);
			nextStep = next_step[opt]["nextStep"];
			sendTo = next_step[opt]["sendTo"];
		} catch (...){
			nextStep = -1;
			sendTo = "e";
			return 0;
		}
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

	int getStorageData(std::string c, std::string s, int& crl, int& key, int& cert, int num_revo_cert){
		json storage;
		try{
			storage = jo[c][s]["storage"];
			int test = storage["crl"];
			return test;
		} catch (...){
			return -1;
		}

		try{
			int tempCrl = storage["crl"];
			tempCrl *= num_revo_cert;
			int tempKey = storage["key"];
			int tempCert = storage["cert"];
			crl += tempCrl;
			if (crl < 0) crl = 0;
			key += tempKey;
			if (key < 0) key = 0;
			cert += tempCert;
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
				accTime = single_Data_access * log10(num_revo);
			} else if (type.compare("CRL") == 0){
				accTime = single_Data_access * num_revo;
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

	std::string getStepName(std::string c, std::string s){
		return jo[c][s]["name"];
	}

	void printAllSteps(std::shared_ptr<EventSerialize> es, std::string lifecycle){
		json ju;
		std::string cycle;
		int i;
		for (auto& x : jo.items()){
			if (x.key().compare("Template") != 0){
				if (x.key().compare("const_data") != 0){
					i = 1;
					cycle = x.key();
					ju = jo[cycle];
					for (json j : ju){
						es->saveInfo(lifecycle, cycle, j["name"], i++, 0, "");

					}
				}
			}
		}
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


#include "MetaData/MetaData.h"
#include "Payload.h"
#include "WTask.h"
#include "NTask.h"
#include "ANode.h"






#endif /* SCRATCH_IOTSIM_SIM_INCLUDE_H_ */
