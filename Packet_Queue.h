/*
 * Packet_Queue.h
 *
 *  Created on: Mar 13, 2020
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_WIFI_PACKET_QUEUE_H_
#define SCRATCH_IOTSIM_WIFI_PACKET_QUEUE_H_



using namespace ns3;
class PacketQueue{

private:
	const std::vector<std::shared_ptr<ANode>> * allNodes;

	int q_id;
	int q_start;
	int runtime;

	int getUnix(){
		std::time_t result = std::time(nullptr);
		return result;
	}

public:

	PacketQueue(int q_id, int runtime){
		this->q_id = q_id;
		q_start = getUnix();
		this->runtime = runtime;
	}

	~PacketQueue(){

	}


	void workQueue(){
		if((getUnix() - q_start) > runtime - 10) return;
		for (auto n : *allNodes){
			n->remote_send();
		}
	}

	void loopQueue(){
		NS_LOG_UNCOND("Looping Queue " + std::to_string(this->q_id));
		while(getUnix() - q_start < runtime){
			workQueue();
		}
	}

	void setAllNodes(const std::vector<std::shared_ptr<ANode> > * allNodes) {
		this->allNodes = allNodes;
	}

};




#endif /* SCRATCH_IOTSIM_WIFI_PACKET_QUEUE_H_ */
