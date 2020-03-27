/*
 * Iot_Sim.cc
 *
 *  Created on: Jul 14, 2019
 *      Author: richard
 */


#include "Sim_Include.h"


using namespace ns3;
using namespace std;

const int RNN = 1;
int INN = 8;
int LNPIN = 6;
int runTime = 900;
int endSimInterval = 10;
int NN;
const int identitySize = 5;
int id_num[identitySize] = {0};
int maxMs = 800;
int init_revoked_certs = 100;
float single_data_access = 0.147;
int wait_to_create_again = 10;
int reCheck = 60;
std::string jsonFile;
std::string simName;
std::vector<shared_ptr<ANode>> allNodes;
int curr_time;
int end_time;
std::shared_ptr<PacketQueue> mainQ_1;
// std::shared_ptr<PacketQueue> mainQ_2;



// std::string jsonFile = "ECC_CRL.json"; std::string simName = "ECC_CRL_";
// std::string jsonFile = "ECC_OSCP.json"; std::string simName = "ECC_OSCP_";
// std::string jsonFile = "RSA_CRL.json";
// std::string jsonFile = "RSA_OSCP.json"; std::string simName = "RSA_OSCP_";
// std::string jsonFile = "Lifecycle_New_Test.json";
// std::string jsonFile = "ECQV_OSCP.json"; std::string simName = "ECQV_OSCP_";





void sigsevHandler(int signum){
	cout << "There was some fuckery in thread " << std::this_thread::get_id() << std::endl;
	exit(signum);
}

void nextId(int i = (identitySize - 1)){
	if (id_num[i] == 9){
		id_num[i] = 0;
		nextId(i - 1);
	} else {
		id_num[i]++;
	}
}

std::string getId(){
	std::stringstream ss("");
	for (int i = 0; i < identitySize; i++){
		ss << id_num[i];
	}
	nextId();
	return ss.str();
}


int main (int argc, char *argv[])
{
	std::string phyMode("DsssRate1Mbps");
	double rss = -70;

	std::vector<int> l_node_indicies;

	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
	Time::SetResolution (Time::NS);
	GlobalValue::Bind("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

	simName = "Default " + std::to_string(INN) + "_" + std::to_string(LNPIN) + "_" + std::to_string(getUnix());
	jsonFile = "No File specified.";

	CommandLine cmd;
	cmd.AddValue("INN", "IntermediantNodeNumber. Number of INodes in the Simulation", INN);
	cmd.AddValue("LNPIN", "LeafNodePerIntermediantNode, Number of LNodes per INode in the Simulation", LNPIN);
	cmd.AddValue("jsonFile", "Json File to import the Lifecycle (with .json)", jsonFile);
	cmd.AddValue("simName", "Name of the Simulation", simName);
	cmd.AddValue("runTime", "Time in Seconds how long the Simulation Runs", runTime);
	cmd.AddValue("maxMs", "Threshhold to count number of to high latency", maxMs);
	cmd.AddValue("single_data_access", "Time in ms it takes to perform a single Data Read = 0.147", single_data_access);
	cmd.AddValue("createWait", "Time in ms to wait after expire to create a new Certificate", wait_to_create_again);
	cmd.AddValue("rss", "RSS for RssLossModel. Default -70", rss);
	cmd.AddValue("reCheck", "Seconds until an node re-checkds the validations of the nodes he has a symmetric key with. Default = 60", reCheck);
	// cmd.AddValue("dataRate", "Datarate in xMbps, Default 5Mbps", dataRate);
	// cmd.AddValue("delay", "Delay in xms, Default 5ms", delay);
	// cmd.AddValue("errorRate", "Error Rate for ErrorModel", errorRate);
	// cmd.AddValue("WTaskRetryTime", "Sets the lower Intervall of when to retry a Waiting Task. Upper Interval is x2 the lower Interval", wt_retry_time);
	cmd.Parse (argc, argv);

	std::shared_ptr<JsonRead> jsonRead(new JsonRead);
	std::shared_ptr<std::mutex> mutex(new std::mutex());
	std::shared_ptr<EventSerialize> es(new EventSerialize("nTaskFinish.csv", "cycleFinish.csv", "info.csv", simName));
	 //(new PacketQueue);
	mainQ_1.reset(new PacketQueue(1, runTime));
	// mainQ_2.reset(new PacketQueue(2, runTime));

	NN = RNN + INN + (INN*LNPIN);

	if (runTime < 180){
		NS_LOG_UNCOND("Simulation needs to run at least for 2 Minutes (180 Seconds)");
		return 0;
	}


	try{
		jsonRead->init(jsonFile, single_data_access);
	} catch (...){
		NS_LOG_UNCOND("Could not import json. " + jsonFile);
		return 0;
	}

	// Creates a NodeContainer for all Nodes in the Wifi
	NodeContainer c;
	c.Create(NN);

	WifiHelper wifi;
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.Set("RxGain", DoubleValue(0));
	wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	// The below FixedRssLossModel will cause the rss to be fixed regardless
	// of the distance between the two stations, and the transmit power
	wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
	wifiPhy.SetChannel (wifiChannel.Create ());

	// Add a mac and disable rate control
	WifiMacHelper wifiMac;
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
								 "DataMode",StringValue (phyMode),
								 "ControlMode",StringValue (phyMode));
	// Set it to adhoc mode
	wifiMac.SetType ("ns3::AdhocWifiMac");
	NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

	// Note that with FixedRssLossModel, the positions below are not
	// used for received signal strength.
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	positionAlloc->Add (Vector (5.0, 0.0, 0.0));
	positionAlloc->Add (Vector (0.0, 5.0, 0.0));
	positionAlloc->Add (Vector (0.0, 0.0, 5.0));
	positionAlloc->Add (Vector (0.0, 0.0, 5.0));
	positionAlloc->Add (Vector (0.0, 5.0, 5.0));
	positionAlloc->Add (Vector (5.0, 5.0, 5.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (c);

	InternetStackHelper internet;
	internet.Install(c);


	Ipv4AddressHelper ipv4;
	Ipv4InterfaceContainer ipv4InterfaceContainer;
	ipv4.SetBase("10.1.0.0", "255.255.0.0");
	ipv4InterfaceContainer = ipv4.Assign(devices);

	int parentIndex;
	// std::shared_ptr<PacketQueue> mainQ = mainQ_1;

	for (int i = 0; i < NN; i++){
		if (i == 0){
			std::shared_ptr<ANode> temp(new ANode(getId(), i, R_NODE, c.Get(i), 0, mutex, jsonRead, es, maxMs, wait_to_create_again, runTime,identitySize, reCheck));
			temp->setNumRevokedCert(init_revoked_certs);
			allNodes.push_back(temp);
		} else {
			if ((i - 1) % (1+LNPIN) == 0){
				parentIndex = i;
				std::shared_ptr<ANode> temp(new ANode(getId(), i, I_NODE, c.Get(i), 0, mutex, jsonRead, es, maxMs, wait_to_create_again, runTime,identitySize, reCheck));
				allNodes.push_back(temp);
			} else {
				std::shared_ptr<ANode> temp(new ANode(getId(), i, L_NODE, c.Get(i), parentIndex, mutex, jsonRead, es, maxMs, wait_to_create_again, runTime,identitySize, reCheck));
				allNodes.push_back(temp);
				l_node_indicies.push_back(i);
			}
		}
	}

	for (int i = 0; i < NN; i++){
		allNodes.at(i)->setAllNodes(&allNodes);
		es->saveInfo(simName,"", "",0,std::stoi(allNodes.at(i)->getId()), allNodes.at(i)->getNodeTypeString());
	}

	mainQ_1->setAllNodes(&allNodes);

	jsonRead->printAllSteps(es, simName);
	uint32_t context1 = 100;
	// uint32_t context2 = 200;

	// Simulator::ScheduleWithContext(context1, Seconds(1), &PacketQueue::start_queue, mainQ_1.get());
	// Simulator::ScheduleWithContext(context2, Seconds(1), &PacketQueue::start_queue, mainQ_2.get());
	// Simulator::ScheduleWithContext(context1, Seconds(1), &PacketQueue::start_queue, mainQ_1.get());
	for (float i = 5.f; i < (runTime-endSimInterval); i+=0.001){
		Simulator::ScheduleWithContext(context1, Seconds(i), &PacketQueue::workQueue, mainQ_1.get());
		// Simulator::ScheduleWithContext(context2, Seconds(i+0.01), &PacketQueue::workQueue, mainQ_2.get());
	}

	NS_LOG_UNCOND("Loaded " + std::to_string(INN) + " I Nodes and " + std::to_string(INN*LNPIN) + " L Nodes.");
	NS_LOG_UNCOND("Simulation runns for " + std::to_string(runTime / 60) + " Minutes");
	NS_LOG_UNCOND("Loading complete. Start the Simulation with ENTER");
	std::cin.ignore();

	for (auto n : allNodes){
		n->startNode();
	}


	Simulator::Stop(Seconds(runTime));
	Simulator::Run();
	Simulator::Destroy();


	for (auto n : allNodes){
		n->killNode("Natural");
		NS_LOG_UNCOND(n->returnStatusReport());
	}
	es->close();
	NS_LOG_UNCOND("Clearing Vector allNodes");
	allNodes.clear();
	NS_LOG_UNCOND("Finish Clearing Vector");

	return 0;
}
























