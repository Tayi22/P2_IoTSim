/*
 * Payload.h
 *
 *  Created on: Feb 2, 2020
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_WIFI_PAYLOAD_H_
#define SCRATCH_IOTSIM_WIFI_PAYLOAD_H_

#include "Sim_Include.h"

class Payload{
public:
	std::string leaf_source_id;
	std::string i_source_id;
	std::string wt_id;
	std::string cycle_id; // Cycle Number
	std::string step_num; // Step Number
	std::string deathPayload;
	std::string affectedNodeIndex;
	ns3::InetSocketAddress next_receipt = "1.1.1.1";
	std::string revoked;

	Payload(int leaf_source_id,
			int i_source_id,
			std::string wt_id,
			std::string cycle_id,
			std::string step_num,
			std::string deathPayload,
			int starter_id)
	: leaf_source_id(std::to_string(leaf_source_id)), i_source_id(std::to_string(i_source_id)), wt_id(wt_id), cycle_id(cycle_id), step_num(step_num), deathPayload(deathPayload), affectedNodeIndex(std::to_string(starter_id)), revoked("0"){}
	Payload() : leaf_source_id(""), i_source_id(""), wt_id(""), cycle_id(""), step_num(""), deathPayload(""), affectedNodeIndex(""), revoked("0"){}

	long readPayloadString(const std::string &payloadString){

		std::vector<std::string> result;
		std::stringstream ss (payloadString);
		std::string item;

	    while (getline (ss, item, ',')) {
	        result.push_back (item);
	    }

	    leaf_source_id = result.at(0);
	    i_source_id = result.at(1);
	    wt_id = result.at(2);
	    cycle_id = result.at(3);
	    step_num = result.at(4);
	    affectedNodeIndex = result.at(5);
	    revoked = result.at(6);
	    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - stol(result.at(6));
	}

	void setNextReceipt(ns3::InetSocketAddress nr){
		this->next_receipt = nr;
	}

	std::string genPayloadString(){
		// float lastUpdate = std::chrono::duration_cast<m<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch().count();
		long lastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::string ret;
		ret = leaf_source_id + "," + i_source_id + "," + wt_id + "," + cycle_id + "," + step_num + "," + affectedNodeIndex + "," + std::to_string(lastUpdate) + "," + revoked + "," + deathPayload;
		return ret;
	}

	std::string to_string(){
		return "Packet: leaf_source_id:" + leaf_source_id + ", i_source_id:" + i_source_id + ", wt_id: " + wt_id + ", cycle_id:" + cycle_id + ", step_num: " + step_num + ", affectedNodeIndex:" + affectedNodeIndex + ", revoked:" + revoked;
	}

	std::string to_string_Payload_size(){
		return "Packet: leaf_source_id: " + leaf_source_id + " i_source_id: " + i_source_id + "wt_id: " + wt_id + " cycle_id: " + cycle_id + " step_num: " + step_num + " affectedNodeIndex: " + affectedNodeIndex + " " + "Payload Size: " + std::to_string(deathPayload.size());
	}

	ns3::InetSocketAddress getNextReceipt() const {
		return next_receipt;
	}

	void setRevoked(std::string r){
		this->revoked = r;
	}

	int getAffectedNodeIndex(){
		return std::stoi(affectedNodeIndex);
	}


	void setDeathPayload(const std::string &deathPayload) {
		this->deathPayload = deathPayload;
	}

	void setStepNum(const std::string &stepNum) {
		step_num = stepNum;
	}

	/*
	bool checkSource(const NodeType & nt, const Payload& pl){
		if (nt == I_NODE){
			if (i_source_id.compare(pl.i_source_id) == 0) return true;
		} else if (nt == L_NODE){
			if (leaf_source_id.compare(pl.leaf_source_id) == 0) return true;
		} else {
			if (starter_id)
		}
		return false;
	}*/

	bool checkStarter(Payload& pl){
		if (this->affectedNodeIndex.compare(pl.affectedNodeIndex) == 0) return true;
		return false;
	}
};





#endif /* SCRATCH_IOTSIM_WIFI_PAYLOAD_H_ */
