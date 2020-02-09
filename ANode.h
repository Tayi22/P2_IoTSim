/*
 * ANode.h
 *
 *  Created on: Feb 1, 2020
 *      Author: richard
 */

#ifndef SCRATCH_IOTSIM_WIFI_ANODE_H_
#define SCRATCH_IOTSIM_WIFI_ANODE_H_

#include "Sim_Include.h"

using namespace ns3;

class ANode{
private:

	// Simple Struct to save Secret Data
	struct Secret{
			bool secretBool;
			bool revoked = false;
			std::chrono::time_point<system_clock> expiresAt;
			int lastExpired = 0;
			// Need to wait at least 10 Seconds until you can create a new one
			const int waitForCreate = 10000;

			bool hasSecret() const {
				return secretBool;
			}

			void setSecret(bool sec) {
				secretBool = sec;
			}

			bool checkExpire(){
				if (!secretBool) return false;
				std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
				if (t > expiresAt){
					return true;
				}
				return false;
			}

			bool isRevoked(){
				return this->revoked;
			}

			void setRevoked(bool r){
				this->revoked = r;
			}

			bool canCreate(int u){
				if (lastExpired == 0) return true;
				if (u - lastExpired > waitForCreate) return true;
				return false;

 			}

			std::string validStringAndExpireIfRevoked(){
				if (revoked){
					expiresAt = std::chrono::system_clock::now();
					return "Revoked";
				}
				if (!secretBool) return "No Secret";
				return "Valid";
			}

			void expireMe(int u){
				secretBool = false;
				revoked = false;
				lastExpired = u;
			}

			void setExpire(int seconds){
				std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
				expiresAt = t + std::chrono::seconds(seconds);
			}

	};

// Variables:
	std::string id;
	std::string reasonOfDeath;
	int index;

	NodeType nodeType;

	Ptr<Node> node;
	Ptr<Socket> send_sock;
	Ptr<Socket> recv_sock;
	int recv_sock_port = 101010;

	int parentIndex;
	const std::vector<std::shared_ptr<ANode>> * allNodes;

	// Ptr<SystemThread> nodeThread;
	bool running;

	std::shared_ptr<std::mutex> m;

	std::list<NTask> nTaskList;
	std::unique_ptr<WTask> wTask_create;
	std::unique_ptr<WTask> wTask_validate;
	std::unique_ptr<WTask> wTask_expire_revoke;

	std::shared_ptr<JsonRead> jsonRead;
	MetaData metaData;
	std::string metaDataPath;
	std::shared_ptr<EventSerialize> es;
	std::shared_ptr<Secret> secret;


	int max_used_ram;
	int maxStorage;
	int currStorage;
	int maxRam;
	int maxMs;
	int ms_over_count;
	int ms_under_count;

// Methods:

	int intRand(const int & min, const int & max){
	    static thread_local mt19937* generator = nullptr;
	    if (!generator) generator = new mt19937(clock() + std::stoi(id));
	    uniform_int_distribution<int> distribution(min, max);
	    return distribution(*generator);
	}

	Payload getPayloadFromPacket(const Ptr<Packet>& pk){

		uint8_t * buffer = new uint8_t[pk->GetSize()];
		pk->CopyData(buffer, pk->GetSize());

		std::string s = std::string(buffer, buffer+(pk)->GetSize());
		Payload pl = Payload();

		try{
			/* long packetTime = */ pl.readPayloadString(s);
			// saveEvent(getUnix(), id, nodeType, PACKET_TRAVEL, "Packet Time Travel", packetTime);
		} catch (char const* msg){
			NS_LOG_UNCOND(id + " cannot extract Payload: " + msg);
		}

		delete buffer;
		return pl;
	}

	void callbackOnRecv(Ptr<Socket> sock){
		Ptr<Packet> pk = sock->Recv();
		if (!running) return;
		Payload pl = getPayloadFromPacket(pk);
		calcStep(pl);
		return;
	}

	void setupSockets(){
		this->send_sock = Socket::CreateSocket(this->node, TypeId::LookupByName("ns3::UdpSocketFactory"));
		this->recv_sock = Socket::CreateSocket(this->node, TypeId::LookupByName("ns3::UdpSocketFactory"));

		// this->send_sock->Bind();
		InetSocketAddress addr = InetSocketAddress(getLocalAddress(),recv_sock_port);

		this->recv_sock->Bind(addr);
		this->recv_sock->SetRecvCallback(MakeCallback(&ANode::callbackOnRecv, this));
	}

	void setParameters(){
		metaData = MetaData(metaDataPath);
		max_used_ram = 0;
		if (this->nodeType == R_NODE){
			this->secret->setSecret(true);
			this->secret->setExpire(99999);
		} else {
			this->secret->setSecret(false);
		}

		currStorage = 0;
		maxStorage = metaData.getData("MAX_STORAGE");
		maxRam = metaData.getData("MAX_RAM");
		ms_over_count = 0;
		ms_under_count = 0;

	}

	void passiveCreate(){
		if (!secret->hasSecret()){
			if (!wTask_create){
				m->try_lock();
				if (secret->canCreate(getUnix())){
					if (nodeType == L_NODE){
						this->startStep("create_L", index);
					} else if (nodeType == I_NODE){
						this->startStep("create_I", index);
					}
				}
				m->unlock();
			}
		}
	}

	void passiveExpire(){
		if (secret->checkExpire()){
			if (!wTask_expire_revoke){
				m->try_lock();
				this->startStep("expire", index);
				m->unlock();
			}
		}
	}

	int checkNTaskList(int max_sim_thread){
		int sim_thread = 1;
		int ram_need = 0;
		float cpu = nTaskList.size() / (float)max_sim_thread;
		if (cpu > 1.f) cpu = 1.f;
		float nTaskTime;
		auto it = nTaskList.begin();
		while (it != nTaskList.end()){
			if (sim_thread > max_sim_thread){
				ram_need += it->getRamNeeded();
				it++;
			} else {
				sim_thread++;
				ram_need += it->getRamNeeded();
				nTaskTime = it->checkTask();
				if (nTaskTime >= 0){
					Payload pl = it->getPl();
					if(it->isSelfSend()){
						this->calcStep(pl);
					} else {
						this->remote_send(pl);
					}
					saveEvent(getUnix(), this->getId(), this->nodeType, NTASK_FINISH, "NTask Finished working", (int)nTaskTime, 0, nTaskList.size(), currStorage, maxStorage, ram_need, maxRam);
					nTaskList.erase(it++);
				} else {
					it++;
				}
			}
		}
		return ram_need;
	}

	void changeParent(){
		m->try_lock();
		for (int i = parentIndex+1; i < (parentIndex + (int)allNodes->size()); i++){
			if (i == (int)allNodes->size()){
				i = 0;
			}
			if (allNodes->at(i)->getNodeType() == I_NODE){
				parentIndex = i;
				NS_LOG_UNCOND(toString() + " found new Parent: " + allNodes->at(parentIndex)->toString());
				break;
			}
		}
		if (wTask_create) wTask_create->getPl().setNextReceipt(allNodes->at(parentIndex)->getInetSocketAddress());
		if (wTask_validate) wTask_validate->getPl().setNextReceipt(allNodes->at(parentIndex)->getInetSocketAddress());
		if (wTask_expire_revoke) wTask_expire_revoke->getPl().setNextReceipt(allNodes->at(parentIndex)->getInetSocketAddress());
		m->unlock();
	}

	void checkWTask(){
		int wTask_status;
		if (wTask_create){
			m->lock();
			wTask_status = wTask_create->check();
			m->unlock();
			if (wTask_status == 1){
				NS_LOG_UNCOND(toString() + " Retry Create");
				saveEvent(getUnix(), id, nodeType, INFO, "Retry Create");
				calcStep(wTask_create->getPl());
			} else if (wTask_status == 2){
				if (nodeType == L_NODE){
					changeParent();
				} else {
					killNode("I think the Root node is ded");
				}
			}
		}
		if (wTask_expire_revoke){
			m->lock();
			wTask_status = wTask_expire_revoke->check();
			m->unlock();
			if (wTask_status == 1){
				NS_LOG_UNCOND(toString() + " Retry Expire Revoke");
				saveEvent(getUnix(), id, nodeType, INFO, "Retry Expire Revoke");
				calcStep(wTask_expire_revoke->getPl());
			} else if (wTask_status == 2){
				if (nodeType == L_NODE){
					changeParent();
				} else {
					killNode("I think the Root node is ded");
				}
			}
		}
		if (wTask_validate){
			m->lock();
			wTask_status = wTask_validate->check();
			m->unlock();
			if (wTask_status == 1){
				NS_LOG_UNCOND(toString() + " Retry Validate");
				saveEvent(getUnix(), id, nodeType, INFO, "Retry Validate");
				calcStep(wTask_validate->getPl());
			} else if (wTask_status == 2){
				if (nodeType == L_NODE){
					changeParent();
				} else {
					killNode("I think the Root node is ded");
				}
			}
		}
	}

	bool checkCondition(std::string con){
		if (con.compare("test") == 0){
			return true;
		}

		NS_LOG_UNCOND("Condition " + con + " not found");
		throw "Condition not found";
	}

	std::string getNodeTypeString(){
		switch (nodeType){
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

	int getUnix(){
		std::time_t result = std::time(nullptr);
		return result;
	}

	void saveEvent(int timestamp, string nodeId, NodeType nodeType, EventType eventType, string event, int completionTime = 0, int running_WT = 0, int running_NT = 0, int storage = 0, int max_storage = 0, int ram = 0, int max_ram = 0){
		if (es!=nullptr){
			es->write(timestamp, nodeId, nodeType, eventType, event, completionTime, running_WT, running_NT, storage, ram, maxStorage, maxRam);
		}
	}

public:
	ANode	(
				std::string id,
				int index,
				NodeType nt,
				Ptr<Node> n,
				int pi,
				std::shared_ptr<std::mutex> m,
				std::shared_ptr<JsonRead> jr,
				std::string metaDataPath,
				std::shared_ptr<EventSerialize> es,
				int maxMs
			):
				id(id),
				index(index),
				nodeType(nt),
				node(n),
				parentIndex(pi),
				m(m),
				jsonRead(jr),
				metaDataPath(metaDataPath),
				es(es),
				maxMs(maxMs)
			{
				this->secret.reset(new Secret());
				setupSockets();
				// this->nodeThread = Create<SystemThread>(MakeCallback(&ANode::loop, this));
				// startThread();
				setParameters();
				reasonOfDeath = "Natural";
				running = true;
				NS_LOG_UNCOND(toString() + " created");
			}

	~ANode(){
		this->nTaskList.clear();
		this->send_sock->Close();
		this->recv_sock->Close();
	}


	void calcStep(Payload pl){
		if (!running) return;

		NS_LOG_INFO(toString() + " working on: " + pl.to_string());

		if (nodeType == I_NODE){

			if (!secret->hasSecret()){
				if (!wTask_create){
					NS_LOG_INFO(toString() + " Cannot accept Packages if not Create Cycle is on the way");
					return;
				} else {
					if (!(wTask_create->getPl().checkStarter(pl))){
						NS_LOG_INFO(toString() + " Cannot comupte Package without Secret");
						return;
					}
				}
			}
		}

		if (this->nodeType == R_NODE || this->nodeType == I_NODE){
			if (allNodes->at(pl.getAffectedNodeIndex())->getSecret()->isRevoked()){
				pl.setRevoked("1");
			} else {
				pl.setRevoked("0");
			}
		}

		float timeNeeded = 0;
		int ramNeeded = 0;
		int needStorage;
		float storageMultiplier = 1.f;
		// std::string storageMultiplierString;
		bool selfSend = false;

		int payloadSize = 0;
		try{
			payloadSize = jsonRead->getDeadPayloadSize(pl.cycle_id, pl.step_num);
			// payloadSize /= 8;
		} catch (char const* msg){
			payloadSize = 1;
		}

		if (payloadSize < 1) payloadSize = 1;
		std::string deadPayload(payloadSize, '1');



		try {
			jsonRead->getStorageData(pl.cycle_id, pl.step_num, needStorage, ramNeeded);
		} catch (char const* msg){
			std::string errString(msg);
			errString = "Could not extract Storage Data in cycleNum/step (getStorageData): " + pl.cycle_id + "/" + pl.step_num + " ErrorString: " + errString;
			NS_LOG_UNCOND(errString);
			saveEvent(getUnix(), this->getId(), this->nodeType, ERROR, getJsonErrString(msg, pl.cycle_id, pl.step_num));
			return;
		}

		if (needStorage != 0){
			currStorage += needStorage;
			if (currStorage < 0) currStorage = 0;
			if (currStorage >= maxStorage){
				saveEvent(getUnix(), this->getId(), this->nodeType, ERROR, "Storage is Full and cannot compute Step");
				return;
			} else {
				// Add some more Time to the StorageMultiplier cause Storage needs more time the more Data is stored.
				storageMultiplier = ((currStorage / maxStorage) + 1);
			}
		}

		try {
			timeNeeded = this->jsonRead->getTime(pl.cycle_id, pl.step_num);
			timeNeeded *= storageMultiplier;
		} catch (char const* msg){
			saveEvent(getUnix(), this->getId(), this->nodeType, ERROR, getJsonErrString(msg, pl.cycle_id, pl.step_num));
			NS_LOG_UNCOND(id + " Could not extract Time Multipliers: cycle/step: " + pl.cycle_id + "/" + pl.step_num);
			return;
		}
		std::string sendTo;
		int nextStep;

		std::string condition;
		int withCon = jsonRead->getCondition(pl.cycle_id, pl.step_num, condition);

		try{
			if (withCon == 0){
				jsonRead->getNextStepCondition(pl.cycle_id, pl.step_num, nextStep, sendTo, checkCondition(condition));
			} else {
				jsonRead->getNextStep(pl.cycle_id, pl.step_num, nextStep, sendTo);
			}
		} catch (char const* msg){
			NS_LOG_UNCOND("There was an error getting the Condition next Step");
			saveEvent(getUnix(), this->getId(), this->nodeType, ERROR, getJsonErrString(msg, pl.cycle_id, pl.step_num));
			return;
		}

		pl.setDeathPayload(deadPayload);
		pl.setStepNum(std::to_string(nextStep));
		InetSocketAddress receipt = "1.1.1.1";

		if (nextStep == -1){
			finishWTask(pl.cycle_id);
		} else if (nextStep == 0){
			saveEvent(getUnix(), this->getId(), this->nodeType, ERROR, "Cycle " + pl.cycle_id + " ended in Step 0");
		} else {
			char switchChar = sendTo[0];
			if (switchChar == 't'){
				selfSend = true;
			} else if (switchChar == 's' || switchChar == 'p' || switchChar == 'a'){
				if (switchChar == 'a'){
					if (pl.revoked.compare("1") == 0){
						finishWTask(pl.cycle_id);
						return;
					}
				}
				try{
					receipt = getReceiverAddress(pl, switchChar);
				} catch (char const* msg){
					NS_LOG_UNCOND(id + "Error while getting Receiver Address" + std::string(msg));
					return;
				}
			} else {
				NS_LOG_UNCOND("Cannot recognize nextStep: " + std::to_string(switchChar));
				return;
			}

			pl.setNextReceipt(receipt);
			NTask nt = NTask(timeNeeded, pl, ramNeeded, selfSend);
			if (pl.cycle_id.compare("expire_revoke") == 0){
				// NS_LOG_UNCOND(toString() + " working on " + pl.to_string());
			}
			NS_LOG_INFO(toString() + " pushing NTask: " + nt.toString());
			this->nTaskList.push_back(nt);
		}


		// remote_send(pl);
	}

	void startStep(std::string cycle_id, int affectedNodeIndex){
		if (!running) return;
		std::shared_ptr<ANode> affectedNode = allNodes->at(affectedNodeIndex);

		Payload pl;
		allNodes->at(affectedNodeIndex)->getNodeType();
		switch(this->nodeType){
		case R_NODE:
			if (allNodes->at(affectedNodeIndex)->getNodeType() == L_NODE){
				pl = Payload(affectedNode->getIndex(), allNodes->at(affectedNode->getParentIndex())->getIndex(), "", cycle_id, "1", "",index);
			} else if (affectedNode->getNodeType() == I_NODE){
				pl = Payload(0, affectedNode->getIndex(), "", cycle_id , "1", "",index);
			} else {
				NS_LOG_UNCOND("R Node targeting itself is not allowed when starting a Cycle. ");
				return;
			}
			break;
		case I_NODE:
			if (affectedNode->getNodeType() == L_NODE){
				pl = Payload(affectedNode->getIndex(), index, "", cycle_id, "1", "",index);
			} else {
				pl = Payload(0, index, "",cycle_id,"1", "", index);
			}
			break;
		case L_NODE:
			pl = Payload(index, 0, "", cycle_id, "1", "",index);
			break;

		default:
			NS_LOG_UNCOND(id + " cant recognize NodeType: " + this->getNodeTypeString());
			return;
			break;
		}
		int wt_retry_time = (int)metaData.getData("WTASK_WAITTIME_SEED");
		int maxTries = metaData.getData("MAX_TRIES");
		float retry_time = intRand(wt_retry_time, wt_retry_time*2);
		m->try_lock();
		if (cycle_id.find("create") != std::string::npos){
			if (!wTask_create){
				wTask_create.reset(new WTask(retry_time, maxTries, pl, affectedNodeIndex));
				// wTask_create = new WTask(retry_time, maxTries, pl, affectedNodeIndex);
				NS_LOG_UNCOND(toString() + " starts create");
				calcStep(pl);
				saveEvent(getUnix(), getId(), getNodeType(), INFO, "WaitTask_Create Created");
			}
		} else if (cycle_id.find("validate") != std::string::npos){
			if (!wTask_validate){
				wTask_validate.reset(new WTask(retry_time, maxTries, pl, affectedNodeIndex));
				NS_LOG_UNCOND(toString() + " starts cycle validate");
				calcStep(pl);
				saveEvent(getUnix(), getId(), getNodeType(), INFO, "WaitTask_Validate Created");

			}
		} else if (cycle_id.compare("expire_revoke") == 0){
			if (!wTask_expire_revoke){
				wTask_expire_revoke.reset(new WTask(retry_time, maxTries, pl, affectedNodeIndex));
				NS_LOG_UNCOND(toString() + " starts cycle expire_revoke");
				calcStep(pl);
				saveEvent(getUnix(), getId(), getNodeType(), INFO, "WaitTask_expire_Revoke Created");
			}
		}
		m->unlock();

		/*
		m->lock();
		std::unique_ptr<WTask> wt(new WTask(affectedNode->getId(), maxTries, cycle_id, wt_id, intRand(wt_retry_time, wt_retry_time * 2), pl));
		this->wTaskList.push_back(std::move(wt));
		m->unlock();
		*/
		//saveEvent(getUnix(), getId(), getNodeType(), INFO, "Waittask created: " + wt_id);

		// calcStep(pl);
	}

	void finishWTask(std::string cycle){
		if (cycle.find("create") != std::string::npos){
			m->try_lock();
			if (wTask_create){
				secret->setSecret(true);
				if (nodeType == L_NODE){
					secret->setExpire(intRand(60,1800));
				} else {
					secret->setExpire(100000);
				}
				NS_LOG_UNCOND(toString() + " got a Secret");
				wTask_create->getAvGTimeAlive() >= maxMs ? ms_over_count++ : ms_under_count++;
				saveEvent(getUnix(), getId(), nodeType, CREATED, "Node got Secret", wTask_create->getTimeAlive());
				wTask_create.reset();
			}
			m->unlock();
			return;
		}
		if (cycle.compare("expire_revoke") == 0){
			m->try_lock();
			if (wTask_expire_revoke){
				int affectedNodeIndex = wTask_expire_revoke->getAffectedNodeIndex();
				wTask_expire_revoke->getAvGTimeAlive() >= maxMs ? ms_over_count++ : ms_under_count++;
				// allNodes->at(wTask_expire_revoke->getAffectedNodeIndex())->getSecret().setSecret(false);
				saveEvent(getUnix(), this->getId(), nodeType, EXPIRED, "Affected Node EXPIRED/REVOKED", wTask_expire_revoke->getTimeAlive());
				NS_LOG_UNCOND(toString() + " revoked/expired Secret");
				allNodes->at(affectedNodeIndex)->getSecret()->expireMe(getUnix());
				wTask_expire_revoke.reset();
			}
			m->unlock();
			return;
		}
		if (cycle.find("validate") != std::string::npos){
			m->try_lock();
			if (wTask_validate){
				int affectedNodeIndex = wTask_validate->getAffectedNodeIndex();
				std::string val = allNodes->at(affectedNodeIndex)->getSecret()->validStringAndExpireIfRevoked();
				std::string msg = toString() + " Validated Node:  " + cycle + " " + allNodes->at(affectedNodeIndex)->toString() + " " + val;
				wTask_validate->getAvGTimeAlive() >= maxMs ? ms_over_count++ : ms_under_count++;
				// allNodes->at(wTask_expire_revoke->getAffectedNodeIndex())->getSecret().setSecret(false);
				saveEvent(getUnix(), this->getId(), nodeType, VALIDATED, msg, wTask_validate->getTimeAlive());
				NS_LOG_UNCOND(msg);
				wTask_validate.reset();
			}
			m->unlock();
			return;
		}
	}


	void remote_send(Payload pl){
		std::string payload = pl.genPayloadString();

		// Ptr<Packet> pkt = Create<Packet>(10);
		Ptr<Packet> pkt = Create<Packet>(reinterpret_cast<const uint8_t*> (payload.c_str()),payload.size());

		//this->send_sock->Connect(pl.getNextReceipt());
		//this->send_sock->Send(pkt);
		this->send_sock->SendTo(pkt, 0, pl.getNextReceipt());
		// NS_LOG_UNCOND(toString() + " Sent now: " + pl.to_string());
	}


	InetSocketAddress getReceiverAddress(Payload& pl, char switchChar){
		int receiver_index = -1;
		switch (this->nodeType){
		case R_NODE:
			if (switchChar == 'p'){
				NS_LOG_INFO("Cannot Send from R_Node to Parent (Would be self Send)");
				throw("R_Node is self Parent");
			} else if (switchChar == 'a'){
				throw("R_Node cannot send to a");
			}
			receiver_index = std::stoi(pl.i_source_id);
			break;
		case I_NODE:
			if (switchChar == 'p'){
				receiver_index = this->parentIndex;
				if (pl.i_source_id.compare("0") == 0) pl.i_source_id = std::to_string(index);
			} else if (switchChar == 's') {
				receiver_index = std::stoi(pl.leaf_source_id);
			} else {
				throw ("I_Node cannot send to a");
			}
			break;
		case L_NODE:
			if (switchChar == 'p'){
				receiver_index = this->parentIndex;
			} else if (switchChar == 'a'){
				if (wTask_validate){
					receiver_index = wTask_validate->getAffectedNodeIndex();
				} else {
					throw ("There is no wTask_Validate active to send to a");
				}
			} else if (switchChar == 's'){
				if (!(pl.leaf_source_id.empty())){
					receiver_index = std::stoi(pl.leaf_source_id);
				} else {
					throw ("Leaf Node cannot send to Sender if no leaf source id is set");
				}
			}
			break;
		}

		if (index <0){
			NS_LOG_UNCOND("No index found in getReceiverAddress");
			throw("No Index found");
		}

		return allNodes->at(receiver_index)->getInetSocketAddress();
	}

	void checkNode(){
		int max_thread = metaData.getData("MAX_THREAD");
		if (running){
			passiveCreate();
			passiveExpire();
			int usedRam = 0;
			try{
				usedRam = checkNTaskList(max_thread);
			} catch (char const* msg){
				killNode(msg);
			}
			if (usedRam > max_used_ram) max_used_ram = usedRam;
			try{
				checkWTask();
			} catch (char const* msg){
				killNode(msg);
			}
		}
	}

	void validate(int affected_node_index, std::string valCycle){
		if (secret->hasSecret()){
			if (!wTask_validate){
				this->startStep(valCycle, affected_node_index);
			}
		}
	}

	void

	void addNTask(NTask nt){
		this->nTaskList.push_back(nt);
	}

	/*
	void stopThread(){
		if (running){
			running = false;
			this->nodeThread->Join();
		}
	}
	*/
	void killNode(std::string msg = "killed"){
		if (running){
			running = false;
			this->reasonOfDeath = msg;
			NS_LOG_UNCOND(toString() + " ended: " + msg + " Max Ram used: " + std::to_string(max_used_ram));
		}
	}

	void revokeNode(){
		this->getSecret()->setRevoked(true);
	}



	std::string toString(){
		return id + " - " + getNodeTypeString() + ":";
	}

	std::string returnStatusReport(){
		int numFinishedCycles = ms_under_count + ms_over_count;
		return toString() + " Cycles: " + std::to_string(numFinishedCycles) + ", over " + std::to_string(maxMs) + "ms " + std::to_string(ms_over_count);
	}


	// Getter + Setter

	Ipv4Address getLocalAddress(){
		return this->node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
	}

	InetSocketAddress getInetSocketAddress(){
		return InetSocketAddress(getLocalAddress(), recv_sock_port);
	}

	void setAllNodes(const std::vector<std::shared_ptr<ANode> > * allNodes) {
		this->allNodes = allNodes;
	}

	bool isType(NodeType nodeType){
		return this->nodeType == nodeType;
	}

	int getIndex() const {
		return index;
	}

	NodeType getNodeType() const {
		return this->nodeType;
	}

	int getParentIndex() const {
		return this->parentIndex;
	}

	std::string getId() const {
		return this->id;
	}

	std::shared_ptr<Secret> getSecret() {
		return secret;
	}

};



#endif /* SCRATCH_IOTSIM_WIFI_ANODE_H_ */
