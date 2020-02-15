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
	std::string source_indices;
	std::string wTaskId;
	std::string cycle_id;
	std::string step_num;
	std::string dead_payload;
	ns3::InetSocketAddress next_receipt = "1.1.1.1";
	std::string affected_node;


	Payload(	int wti,
				std::string ci,
				int sn,
				int an
			) :
				wTaskId(std::to_string(wti)),
				cycle_id(ci),
				step_num(std::to_string(sn)),
				affected_node(std::to_string(an))
		{
				dead_payload = "";
				source_indices = "";
		}

	Payload(const std::string &payloadString){
		std::vector<std::string> result;
		std::stringstream ss (payloadString);
		std::string item;

		while (getline (ss, item, ',')) {
			result.push_back (item);
		}

		source_indices = result.at(0);
		wTaskId = result.at(1);
		cycle_id = result.at(2);
		step_num = result.at(3);
		affected_node = result.at(4);
		dead_payload = "";
	}


	void addSource(int node_index){
		if (source_indices.empty()){
			source_indices = std::to_string(node_index);
		} else {
			source_indices += ("-" + std::to_string(node_index));
		}
	}

	void remSource(int node_index){
		if (source_indices.empty()){
			throw "Cannot remove a Source from an empty Source indices String";
		}

		size_t pos = source_indices.find_last_of("-");
		if (pos == std::string::npos){
			source_indices = "";
		} else {
			int lastIndex = std::stoi(source_indices.substr(pos+1));
			if (node_index == lastIndex) source_indices = source_indices.substr(0, pos);
		}
	}

	int getLastSourceIndex(){
		if (source_indices.empty()){
			throw ("Source Index is empty");
		}
		size_t pos = source_indices.find_last_of("-");
		if (pos == std::string::npos){
			return std::stoi(source_indices);
		} else {
			return std::stoi(source_indices.substr(pos+1));
		}
	}

	void setNextReceipt(ns3::InetSocketAddress nr){
		this->next_receipt = nr;
	}

	ns3::InetSocketAddress getNextReceipt() const {
		return next_receipt;
	}

	std::string genPayloadString(){
		return source_indices + "," + wTaskId + "," + cycle_id + "," + step_num + "," + affected_node + "," + dead_payload;
	}

	std::string toString(){
		return "Source Indices:" + source_indices + ", wTaskId: " + wTaskId + ", Cycle Id:" + cycle_id + ", " + "Step Num:" + step_num + ", Affected Node:" + affected_node + ", Dead Payload Size:" + std::to_string(dead_payload.size());
	}

	void setDeathPayload(const std::string &deathPayload) {
		this->dead_payload = deathPayload;
	}

	void setStepNum(const std::string &stepNum) {
		step_num = stepNum;
	}

	int getAffectedNode() const {
		return std::stoi(affected_node);
	}
};





#endif /* SCRATCH_IOTSIM_WIFI_PAYLOAD_H_ */
