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

	struct QueueObj{
		Ptr<Socket> sock;
		Payload pl;
	};

	std::queue<QueueObj> mainQ;
	bool running = false;
	Ptr<SystemThread> queueThread;

	void workLoop(){
		while(running){
			workQueue();
		}
	}

public:

	PacketQueue(){
		mainQ = std::queue<QueueObj>();
	}

	void addQueue(Payload pl, Ptr<Socket> sock){
		QueueObj temp;
		temp.sock = sock;
		temp.pl = pl;
		mainQ.push(temp);
	}

	void workQueue(){
		if (mainQ.empty()) return;
		QueueObj temp = mainQ.front();
		std::string payload = temp.pl.genPayloadString();
		Ptr<Packet> pkt = Create<Packet>(reinterpret_cast<const uint8_t*> (payload.c_str()),payload.size());
		temp.sock->SendTo(pkt, 0, temp.pl.getNextReceipt());
		mainQ.pop();
	}

	void runQueue(){
		running = true;
		queueThread = Create<SystemThread>(MakeCallback(&PacketQueue::workLoop, this));
		queueThread->Start();
		return;
	}

	void stopQueue(){
		running = false;
	}


};




#endif /* SCRATCH_IOTSIM_WIFI_PACKET_QUEUE_H_ */
