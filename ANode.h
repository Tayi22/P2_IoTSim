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

int getUnix(){
	std::time_t result = std::time(nullptr);
	return result;
}

// When a NodeData needs to be rechecked in Seconds.
const int reCheck = 20;

class ANode{
private:

	struct StorageData{
		int key = 0;
		int cert = 0;
		int crl = 0;

		int getStorage(){
			return key + cert + crl;
		}
	};

	struct NodeData{
		bool isValid;
		bool hasCrl;
		int lastCheck;


		NodeData(){
			isValid = false;
			hasCrl = false;
			lastCheck = getUnix();
		}

		bool checkNew(){
			if (getUnix() - lastCheck > reCheck) return true;
			return false;
		}

	};

	// Simple Struct to save Secret Data
	struct Secret{
			bool secretBool;
			bool revoked = false;
			std::chrono::time_point<system_clock> expiresAt;
			int lastExpired = 0;
			// Need to wait at least 60b Seconds until you can create a new one
			int waitForCreate = 60;
			// std::vector<int> valid_l_nodes;

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
				if ((u - lastExpired) > waitForCreate) return true;
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
	/*
	std::unique_ptr<WTask> wTask_create;
	std::unique_ptr<WTask> wTask_validate;
	std::unique_ptr<WTask> wTask_expire_revoke;
	*/

	std::map<int, WTask> wTaskMap;
	int nextId = 0;

	std::shared_ptr<JsonRead> jsonRead;
	MetaData metaData;
	std::string metaDataPath;
	std::shared_ptr<EventSerialize> es;
	std::shared_ptr<Secret> secret;


	int max_used_ram;
	int maxStorage;
	StorageData storage_data;
	int maxRam;
	int maxMs;
	int ms_over_count;
	int ms_under_count;
	int num_revoked_cert;

	long unsigned int cpu_ticks;
	float sec_ticks;

	std::map<int, NodeData> allNodeData;


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
		Payload pl;

		try{
			pl = Payload(s);
		} catch (char const* msg){
			NS_LOG_UNCOND(toString() + " cannot extract Payload: " + msg);
		} catch (...){
			NS_LOG_UNCOND(toString() + " cannot extract Payload");
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

		maxStorage = metaData.getData("MAX_STORAGE");
		maxRam = metaData.getData("MAX_RAM");
		ms_over_count = 0;
		ms_under_count = 0;

		cpu_ticks = 0;
		sec_ticks = 0;

	}

	std::map<int, WTask>::iterator getWTask(int aff_node){
		std::map<int, WTask>::iterator it;
		it = wTaskMap.find(aff_node);
		if (it == wTaskMap.end()){
			return nullptr;
		}
		return it;
	}

	bool canCreatePassive(std::string cylce_id){
		std::map<int, WTask>::iterator it;
		it = wTaskMap.find(index);
		if (it == wTaskMap.end()){
			return true;
		}
		if (it->second.getPl().cycle_id.find(cylce_id) != std::string::npos){
			return true;
		}
		return false;
	}

	void passiveCreate(){
		if (!secret->hasSecret()){
			if (canCreatePassive("create")){
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
			WTask wt;
			if (canCreatePassive("validate")){
				m->try_lock();
				this->startStep("expire_revoke", index);
				m->unlock();
			}
		}
	}

	int checkNTaskList(unsigned int max_sim_thread){
		if (nTaskList.empty()) return 0;
		unsigned int sim_thread = 1;
		int ram_need = 0;
		if (nTaskList.size() >= max_sim_thread){
			sec_ticks += 1.f;
		} else {
			sec_ticks += (float) nTaskList.size() / (float)max_sim_thread;
		}
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
					saveNTaskFinish(getUnix(), getId(), nodeType, NTASK_FINISH, it->toString(), (int)nTaskTime, sec_ticks/cpu_ticks, nTaskList.size(), storage_data.getStorage(), maxStorage, ram_need, maxRam);
					nTaskList.erase(it++);
				} else {
					it++;
				}
			}
		}
		return ram_need;
	}

	void changeParent(){
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

	}

	void checkWTask(){
		int wTask_status;
		std::map<int, WTask>::iterator it;

		m->try_lock();
		for (it = wTaskMap.begin(); it != wTaskMap.end(); it++){
			wTask_status = it->second.check();

			if (wTask_status == 1){
				NS_LOG_UNCOND(toString() + " Retry Create");
				calcStep(it->second.getPl());
			} else if (wTask_status == 2){
				if (nodeType == L_NODE){
					changeParent();
				} else {
					killNode("I think the Root node is ded");
				}
			}
		}
		m->unlock();
	}

	int checkCondition(std::string con, int aff_node){
		if (con.empty()) return 1;
		if (con.compare("has_Crl") == 0){
			std::map<int, NodeData>::iterator it;
			it = allNodeData.find(aff_node);
			if (it == allNodeData.end()){
				return 1;
			}
			if (it->second.hasCrl){
				return 2;
			}
			return 1;
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
	// enum EventType {INFO, CREATED, VALIDATED, ERROR, NTASK_FINISH, WTASK_FINISH, EXPIRED, REVOKED, PACKET_TRAVEL};
	std::string getEventTypeString(EventType et){
		switch (et){
		case INFO:
			return "Info";
		case CREATED:
			return "Create";
		case VALIDATED:
			return "Validate";
		case ERROR:
			return "Error";
		case NTASK_FINISH:
			return "NTask_Finish";
		case EXPIRED:
			return "Expire";
		case WTASK_FINISH:
			return "WTask_Finish";
		case REVOKED:
			return "Revoke";
		case PACKET_TRAVEL:
			return "Packet_Travel";
		default:
			return "Not Know";
		}
	}



	void saveNTaskFinish(int timestamp, string nodeId, NodeType nodeType, EventType eventType, string event, int completionTime, float cpu, int running_NT, int storage, int max_storage, int ram, int max_ram){
		if (es){
			std::string nodeType_string = getNodeTypeString();
			std::string eventType_string = getEventTypeString(eventType);
			es->saveNTaskFinish(timestamp, nodeId, nodeType_string, eventType_string, event, completionTime, cpu, running_NT, storage, max_storage, ram, max_ram);
		}
	}

	void saveInfo(int timestamp, string nodeId, NodeType nodeType, EventType eventType, string event){
		if (es){
			std::string nodeType_string = getNodeTypeString();
			std::string eventType_string = getEventTypeString(eventType);
			es->saveInfo(timestamp, nodeId, nodeType_string, eventType_string, event);
		}
	}

	void saveCycleFinish(int timestamp, string nodeId, NodeType nodeType, EventType eventType, string event, int completionTime, int tries){
		if (es){
			std::string nodeType_string = getNodeTypeString();
			std::string eventType_string = getEventTypeString(eventType);
			es->saveCycleFinish(timestamp, nodeId, nodeType_string, eventType_string, event, completionTime, tries);
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
				int maxMs,
				int wait_to_create_again
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
				this->secret->waitForCreate = wait_to_create_again;
				setupSockets();
				// this->nodeThread = Create<SystemThread>(MakeCallback(&ANode::loop, this));
				// startThread();
				setParameters();
				reasonOfDeath = "Natural";
				running = true;
				num_revoked_cert = 0;
				NS_LOG_UNCOND(toString() + " created");
			}

	~ANode(){
		this->nTaskList.clear();
		this->send_sock->Close();
		this->recv_sock->Close();
	}


	void calcStep(Payload pl){
		if (!running) return;
		NS_LOG_INFO(toString() + " working on: " + pl.toString());

		if (nodeType == I_NODE){
			if (!secret->hasSecret()){
				if (wTaskMap.count(pl.getAffectedNode()) == 0){
					NS_LOG_INFO("Cannot Compute Package until i have a Secret");
					return;
				}
			}
		}

		float time_need;
		int ram_need;
		float time_data_access = 0;
		bool self_send = false;
		int num_revoked_cert_temp = allNodes->at(0)->getNumRevokedCert();
		bool check_valid = false;
		std::string sendTo;
		int nextStep;
		int option = 1;
		std::string condition = "";

		// Check of a Payload Size is specified and build a string with payloadsize bytes or 1.
		int payload_size = jsonRead->getDeadPayloadSize(pl.cycle_id, pl.step_num, num_revoked_cert_temp);
		if (payload_size < 0){
			NS_LOG_DEBUG("No Payload in PL: " + pl.toString());
		}

		std::string dead_payload(payload_size, '1');

		// Get critical information for the next task. Fails the Simulation if not found.
		try{
			jsonRead->getTimeRam(pl.cycle_id, pl.step_num, time_need, ram_need);
		} catch (...){
			NS_LOG_UNCOND(pl.toString() + " doesnt include must have data time or ram_strain");
			throw ("Critical error in Simulation in getTimeRam from calcStep");
		}

		// Get Information about the storage usage (cert, crl, key). If no storage field is specified -> No Problem. If a storage field is specified but an element is missing it creates a critical Error
		try{
			jsonRead->getStorageData(pl.cycle_id, pl.step_num, storage_data.crl, storage_data.key, storage_data.cert);
		} catch (...){
			NS_LOG_UNCOND(pl.toString() + " does include Storage but is missing crl, key or cert field");
			throw ("Critical error in Simulation in getStorageData from calcStep");
			return;
		}

		// Get Additional Time to access data.
		if (jsonRead->getAccTime(pl.cycle_id, pl.step_num, time_data_access, num_revoked_cert_temp) < 0){
			NS_LOG_DEBUG("No check_revoke_list field in " + pl.toString());
		}

		time_need += time_data_access;

		// Check if we need to see if the affected node has a secret.
		if (jsonRead->checkvalid(pl.cycle_id, pl.step_num, check_valid) < 0){
			NS_LOG_DEBUG("No check_valid_add field in " + pl.toString());
		}

		// Get a Condition if there is one
		if (jsonRead->getCondition(pl.cycle_id, pl.step_num, condition) < 0){
			NS_LOG_DEBUG("No condition found in " + pl.toString());
		}

		// Check if we need to switch from Option 1 to 2. Can critical Fail if a stated condition is not implemented.
		option = checkCondition(condition, pl.getAffectedNode());

		// Get the next Step. Critical Error if a found next_step object doesnt contain sendTo + nextStep
		try{
			jsonRead->getNextStep(pl.cycle_id, pl.step_num, option, nextStep, sendTo);
		} catch (...){
			NS_LOG_UNCOND("Critical Error while getting Next Step. ");
			throw ("Critical Error while getting NextStep");
		}


		// Check if the affected Node has a valid Secret or finish the current WTask;
		if (check_valid){
			if (!(allNodes->at(pl.getAffectedNode())->getSecret()->hasSecret())){
				finishWTask(pl.getAffectedNode());
				return;
			}
		}

		pl.setDeathPayload(dead_payload);
		pl.setStepNum(std::to_string(nextStep));
		InetSocketAddress receipt = "1.1.1.1";

		if (nextStep == -1){
			finishWTask(pl.getAffectedNode());
		} else {
			char switchChar = sendTo[0];
			if (switchChar == 't'){
				self_send = true;
			} else if (switchChar == 's' || switchChar == 'p' || switchChar == 'a' || switchChar == 'r'){
				try{
					receipt = getReceiverAddress(pl, switchChar);
				} catch (char const* msg){
					NS_LOG_UNCOND(toString() + " Error while getting Receiver Address: " + std::string(msg));
					return;
				}
			} else {
				NS_LOG_UNCOND("Cannot recognize nextStep: " + std::to_string(switchChar));
				return;
			}

			pl.setNextReceipt(receipt);
			NTask nt = NTask(time_need, pl, ram_need, self_send);

			//NS_LOG_INFO(toString() + " pushing NTask: " + nt.toString());
			this->nTaskList.push_back(nt);
		}
	}

	void startStep(std::string cycle_id, int affectedNodeIndex){
		if (!running) return;
		if (wTaskMap.count(affectedNodeIndex) != 0){
			NS_LOG_DEBUG("Cannot Start a new Cycle if a Cycle with the same affected node is already running");
			return;
		}

		m->try_lock();
		std::shared_ptr<ANode> aff_node = allNodes->at(affectedNodeIndex);
		Payload pl = Payload(nextId++, cycle_id, 1, affectedNodeIndex);

		int wt_retry_time = (int)metaData.getData("WTASK_WAITTIME_SEED");
		int maxTries = metaData.getData("MAX_TRIES");
		float retry_time = intRand(wt_retry_time, wt_retry_time*2);

		WTask temp = WTask(retry_time, maxTries, pl);
		wTaskMap.insert(std::pair<int, WTask>(affectedNodeIndex, temp));
		calcStep(pl);
		m->unlock();
	}

	void finishWTask(int aff_node){
		std::map<int, WTask>::iterator it;
		it = wTaskMap.find(aff_node);
		if (it == wTaskMap.end()){
			NS_LOG_UNCOND(toString() + " didnt have a WTask waiting for " + std::to_string(aff_node));
			return;
		}

		m->try_lock();
		WTask wt = it->second;
		std::string cycle_id = wt.getCycleId();

		if (cycle_id.find("create") != std::string::npos){
			secret->setSecret(true);
			if (nodeType == L_NODE){
				secret->setExpire(intRand(60,1800));
			} else {
				secret->setExpire(100000);
			}
			NS_LOG_UNCOND(toString() + " got a Secret");
			wt.getTimeSuccess() >= maxMs ? ms_over_count++ : ms_under_count++;
			saveCycleFinish(getUnix(), id, nodeType, CREATED, "Node got Secret", wt.getTimeSuccess(), wt.getTries());

		} else if(cycle_id.compare("expire_revoke") == 0) {
			wt.getTimeSuccess() >= maxMs ? ms_over_count++ : ms_under_count++;
			saveCycleFinish(getUnix(), id, nodeType, EXPIRED, "Node Expired", wt.getTimeSuccess(), wt.getTries());
			NS_LOG_UNCOND(toString() + " revoked/expired Secret");
			allNodes->at(aff_node)->getSecret()->expireMe(getUnix());

		} else if (cycle_id.find("validate") != std::string::npos){

		}

		wTaskMap.erase(it);
		m->unlock();

	}


	void remote_send(Payload pl){
		std::string payload = pl.genPayloadString();
		Ptr<Packet> pkt = Create<Packet>(reinterpret_cast<const uint8_t*> (payload.c_str()),payload.size());
		this->send_sock->SendTo(pkt, 0, pl.getNextReceipt());
	}


	InetSocketAddress getReceiverAddress(Payload& pl, char switchChar){
		int receiver_index = -1;
		switch (this->nodeType){
		case R_NODE:
			if (switchChar != 's'){
				NS_LOG_UNCOND(toString() + " is not allowed to send to p/a/r");
				throw("R_Node is self Parent");
			}
			receiver_index = pl.getLastSourceIndex();
			break;
		case I_NODE:
			if (switchChar == 'a' || switchChar == 'r'){
				NS_LOG_UNCOND(toString() + " is not allowed to send to a/r (For r use p)");
				throw("I_Node wrong SendTo");
			}
			if (switchChar == 'p'){
				receiver_index = this->parentIndex;
				pl.addSource(index);
			} else if (switchChar == 's') {
				receiver_index = pl.getLastSourceIndex();
				pl.remSource(index);
			}
			break;
		case L_NODE:
			if (switchChar == 'p'){
				receiver_index = this->parentIndex;
			} else if (switchChar == 'a'){
				receiver_index = pl.getAffectedNode();
				pl.addSource(index);
			} else if (switchChar == 's'){
				receiver_index = pl.getLastSourceIndex();
				pl.remSource(index);
			} else if (switchChar == 'r'){
					receiver_index = 0;
			}
			break;
		}

		if (index < 0){
			NS_LOG_UNCOND("No index found in getReceiverAddress");
			throw("No Index found");
		}

		return allNodes->at(receiver_index)->getInetSocketAddress();
	}

	void checkNode(){
		unsigned int max_thread = metaData.getData("MAX_THREAD");
		if (running){
			cpu_ticks++;
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

	void workOnNode(int affected_node){
		sec_ticks++;
		// TODO - Implement;
	}
	/*
	void validate2(int affected_node_index, std::string valCycle){
		this->startStep(valCycle, affected_node_index);
	}*/



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
		NS_LOG_UNCOND(toString() + "got Revoked");
		allNodes->at(0)->plus_revoked();
		if (this->getSecret()->hasSecret()) this->getSecret()->setRevoked(true);
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

	void setNumRevokedCert(int numRevokedCert) {
		num_revoked_cert = numRevokedCert;
	}

	int getNumRevokedCert() const {
		return num_revoked_cert;
	}

	void plus_revoked(){
		num_revoked_cert++;
	}



};



#endif /* SCRATCH_IOTSIM_WIFI_ANODE_H_ */
