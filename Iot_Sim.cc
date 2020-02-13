/*
 * Iot_Sim.cc
 *
 *  Created on: Jul 14, 2019
 *      Author: richard
 */


/*
 * Information to INode:
 * INodes sendTo1 sends Data to Root Node
 * INodes sendTo2 sends Data to Leaf Node
 * LNodes sendTo1 sends Data to Intermediant Node
 * RNotes sendTo1 sends Data to Intermediant Node
 */

#include "Sim_Include.h"


using namespace ns3;
using namespace std;

const int RNN = 1;
int INN = 5;
int LNPIN = 10;
int runTime = 1800;
int endSimInterval = 10;
int NN;
const int identitySize = 5;
int id_num[identitySize] = {0};
int maxMs = 500;
const int init_revoked_certs = 100;

std::string jsonFile = "Lifecycle_ECC_Explicit_OSCP.json";



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
	std::vector<shared_ptr<ANode>> allNodes;
	std::vector<int> l_node_indicies;

	std::shared_ptr<JsonRead> jsonRead(new JsonRead);
	std::shared_ptr<std::mutex> mutex(new std::mutex());
	std::shared_ptr<EventSerialize> es(new EventSerialize("nTaskFinish.csv", "cycleFinish.csv", "info.csv"));

	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
	Time::SetResolution (Time::NS);
	GlobalValue::Bind("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));

	CommandLine cmd;
	cmd.AddValue("INN", "IntermediantNodeNumber. Number of INodes in the Simulation", INN);
	cmd.AddValue("LNPIN", "LeafNodePerIntermediantNode, Number of LNodes per INode in the Simulation", LNPIN);
	cmd.AddValue("jsonFile", "Json File to import the Lifecycle (with .json)", jsonFile);
	cmd.AddValue("runTime", "Time in Seconds how long the Simulation Runs", runTime);
	cmd.AddValue("maxMs", "Threshhold to count number of to high latency", maxMs);
	// cmd.AddValue("dataRate", "Datarate in xMbps, Default 5Mbps", dataRate);
	// cmd.AddValue("delay", "Delay in xms, Default 5ms", delay);
	// cmd.AddValue("errorRate", "Error Rate for ErrorModel", errorRate);
	// cmd.AddValue("WTaskRetryTime", "Sets the lower Intervall of when to retry a Waiting Task. Upper Interval is x2 the lower Interval", wt_retry_time);
	cmd.Parse (argc, argv);

	NN = RNN + INN + (INN*LNPIN);

	jsonRead->init(jsonFile);

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

	for (int i = 0; i < NN; i++){
		if (i == 0){
			std::shared_ptr<ANode> temp(new ANode(getId(), i, R_NODE, c.Get(i), 0, mutex, jsonRead, "R_Node_Metadata.txt", es, maxMs));
			temp->setNumRevokedCert(init_revoked_certs);
			allNodes.push_back(std::move(temp));
		} else {
			if ((i - 1) % (1+LNPIN) == 0){
				parentIndex = i;
				std::shared_ptr<ANode> temp(new ANode(getId(), i, I_NODE, c.Get(i), 0, mutex, jsonRead, "I_Node_Metadata.txt", es, maxMs));
				allNodes.push_back(std::move(temp));
			} else {
				std::shared_ptr<ANode> temp(new ANode(getId(), i, L_NODE, c.Get(i), parentIndex, mutex, jsonRead, "L_Node_Metadata.txt", es, maxMs));
				allNodes.push_back(std::move(temp));
				l_node_indicies.push_back(i);
			}
		}
	}

	for (int i = 0; i < NN; i++){
		allNodes.at(i)->setAllNodes(&allNodes);
	}

	// Setting Events

	// Simulator::Schedule(Seconds(1.0), &ANode::killNode, allNodes.at(4).get(), "Never lived");

	for (float i = 5.f; i < (runTime-endSimInterval); i+=0.02){
		for (auto n : allNodes) {
			Simulator::Schedule(Seconds(i), &ANode::checkNode, n.get());
		}
	}

	int n1;
	int n2;
	int l1;
	int l2;

	for (int i = 5; i<(runTime-endSimInterval); i++){
		for (int u = 0; u < 5; u++){
			srand(i);
			n1 = rand() % l_node_indicies.size();
			n2 = rand() % l_node_indicies.size();
			if (n1 != n2){
				l1 = l_node_indicies.at(n1);
				l2 = l_node_indicies.at(n2);
				if (i % 2 == 0){
					Simulator::Schedule(Seconds(i), &ANode::validate, allNodes.at(l1).get(),l2, "validate_KE");
					NS_LOG_INFO("Scheduled Validate Starter: " + allNodes.at(l1)->toString() + " wants to validate_ECDH " + allNodes.at(l2)->toString());
				} else {
					Simulator::Schedule(Seconds(i), &ANode::validate, allNodes.at(l1).get(),l2, "validate_Ident");
					NS_LOG_INFO("Scheduled Validate Starter: " + allNodes.at(l1)->toString() + " wants to validate_OCSP " + allNodes.at(l2)->toString());
				}
			}
		}

	}


	for (auto n : allNodes) {
		Simulator::Schedule(Seconds(runTime-(endSimInterval + 1)), &ANode::killNode, n.get(), "Natural");
	}

	Simulator::Schedule(Seconds(60), &ANode::revokeNode, allNodes.at(3).get());



	// End Setting Events

	/*
	for (auto n : allNodes) {
		if (!(n->isType(R_NODE))){
			Simulator::Schedule(Seconds(3.0), &ANode::startStep, n.get(), "create", n->getIndex());
		}
	}

	for (auto n : allNodes) {
		if (n->isType(L_NODE)){
			Simulator::Schedule(Seconds(7.0), &ANode::startStep, n.get(), "create", n->getIndex());
		}
	}
*/

	NS_LOG_UNCOND("Loaded " + std::to_string(INN) + " I Nodes and " + std::to_string(INN*LNPIN) + " L Nodes.");
	NS_LOG_UNCOND("Simulation runns for " + std::to_string(runTime / 60) + " Minutes");
	NS_LOG_UNCOND("Loading complete. Start the Simulation with ENTER");
	std::cin.ignore();



	Simulator::Stop(Seconds(runTime));
	Simulator::Run();
	Simulator::Destroy();


	for (auto n : allNodes){
		NS_LOG_UNCOND(n->returnStatusReport());
	}

	NS_LOG_UNCOND("Clearing Vector allNodes");
	allNodes.clear();
	es->close();

	return 0;


}
























