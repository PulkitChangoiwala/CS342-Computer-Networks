#include <bits/stdc++.h>
#define num_sender 3
#define Num_Packets 10000000
#define KB 1024
typedef uint32_t uint;
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"
using namespace ns3;



NS_LOG_COMPONENT_DEFINE ("CS342 Assignment-4 Group-5");

//Declaration of all the functions used in the program
PointToPointHelper help_configP2P(std::string bandwidth, std::string delay, std::string queuesize);
static void congestWindowSize(Ptr<OutputStreamWrapper> stream, double startTime, uint oldCwnd, uint newCwnd);
static void dropPackets(Ptr<OutputStreamWrapper> stream, double startTime, uint flow_id);
void goodput_calculate(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, const Address& addr);
void throughput_calculate(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint interface);
Ptr<Socket> Create_Socket(Address sinkAddress, 
					uint sinkPort, 
					std::string tcpVariant, 
					Ptr<Node> hostNode, 
					Ptr<Node> sinkNode, 
					double startTime, 
					double stopTime,
					uint packetSize,
					uint numPackets,
					std::string dataRate,
					double appStartTime,
					double appStopTime);
void Initialize(Ipv4InterfaceContainer* Receiver_Interfaces1,NodeContainer* Senders1,NodeContainer* Receivers1,int* Packet_Size1);

//Declaration of the global variables used in the program
std::map<uint, uint> dropped_packets; 
std::map<Address, double> total_bytes_gput;			  
std::map<std::string, double> total_bytes_tput;		  
std::map<std::string, double> max_throughput;		  
char types[num_sender][40]={"TcpHybla","TcpWestwood+","TcpYeah"};
double tput_kbps;
double gput_kbps;

/* The definition of class APP and all its associated functions has been copied from ns-3 tutorial */

class APP: public Application {
private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void ScheduleTx(void);
	void SendPacket(void);

	Ptr<Socket>     mSocket;
	Address         mPeer;
	uint32_t        mPacketSize;
	uint32_t        mNPackets;
	DataRate        mDataRate;
	EventId         mSendEvent;
	bool            mRunning;
	uint32_t        mPacketsSent;

public:
	APP();
	virtual ~APP();

	void Setup(Ptr<Socket> socket, Address address, uint packetSize, uint nPackets, DataRate dataRate){
		mSocket = socket;
		mPeer = address;
		mPacketSize = packetSize;
		mNPackets = nPackets;
		mDataRate = dataRate;
	};
	void ChangeRate(DataRate newRate){
		mDataRate = newRate;
		return;
	};
		// void recv(int numBytesRcvd);

};

APP::APP(): mSocket(0),
mPeer(),
mPacketSize(0),
mNPackets(0),
mDataRate(0),
mSendEvent(),
mRunning(false),
mPacketsSent(0) {
}

APP::~APP() {
	mSocket = 0;
}

void APP::StartApplication() {
	mRunning = true;
	mPacketsSent = 0;
	mSocket->Bind();
	mSocket->Connect(mPeer);
	SendPacket();
}

void APP::StopApplication() {
	mRunning = false;
	if(mSendEvent.IsRunning()) {
		Simulator::Cancel(mSendEvent);
	}
	if(mSocket) {
		mSocket->Close();
	}
}

void APP::SendPacket() {
	Ptr<Packet> packet = Create<Packet>(mPacketSize);
	mSocket->Send(packet);

	if(++mPacketsSent < mNPackets) {
		ScheduleTx();
	}
}



void APP::ScheduleTx() {
	if (mRunning) {
		Time tNext(Seconds(mPacketSize*8/static_cast<double>(mDataRate.GetBitRate())));
		mSendEvent = Simulator::Schedule(tNext, &APP::SendPacket, this);
	}
}


PointToPointHelper help_configP2P(std::string bandwidth, std::string delay, std::string queuesize)
{
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue(bandwidth)); //set bandwidth
	p2p.SetChannelAttribute("Delay", StringValue(delay));		//set delay
	p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(queuesize));	//set queue size
	return p2p;
}

//Stores time vs congestion window size in file
static void congestWindowSize(Ptr<OutputStreamWrapper> stream, double startTime, uint oldCwnd, uint newCwnd){

	// storing time elapsed till now and current congestion window size into given filestream
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << newCwnd << std::endl; 
}


static void dropPackets(Ptr<OutputStreamWrapper> stream, double startTime, uint flow_id) {

	//if a packet is getting dropped due to buffer overflow, we store the time of drop in the file
	*stream->GetStream() << Simulator::Now ().GetSeconds () - startTime << "\t" << std::endl; 
	if(dropped_packets.find(flow_id) == dropped_packets.end()) {  //this flow got a packet dropped for the first time
		dropped_packets[flow_id] = 0;							  // hence initialising to zero
	}
	dropped_packets[flow_id]+=1; //incrementing the number of dropped packets
}


// Function to calculate goodput and store the data into file
void goodput_calculate(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, const Address& addr){

	double current_time = Simulator::Now().GetSeconds();
	if(total_bytes_gput.find(addr) == total_bytes_gput.end()) //if first packet received by flow
		total_bytes_gput[addr] = 0;

	total_bytes_gput[addr] += p->GetSize();					  //adding current packet size
	gput_kbps = (((total_bytes_gput[addr] * 8.0) / KB)/(current_time-startTime)); //calculating goodput
	*stream->GetStream() << current_time-startTime << "\t" <<  gput_kbps << std::endl;	   //storing in file
}

// Function to calculate throughput and store the data into file
void throughput_calculate(Ptr<OutputStreamWrapper> stream, double startTime, std::string context, Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint interface) {

	double current_time = Simulator::Now().GetSeconds();

	//if first packet received by flow
	if(total_bytes_tput.find(context) == total_bytes_tput.end())
		total_bytes_tput[context] = 0;
	if(max_throughput.find(context) == max_throughput.end())
		max_throughput[context] = 0;

	total_bytes_tput[context] += p->GetSize();			//adding current packet size
	tput_kbps = (((total_bytes_tput[context] * 8.0) / KB)/(current_time-startTime)); //calculating throughput
	*stream->GetStream() << current_time-startTime << "\t" <<  tput_kbps << std::endl;	      //storing in file
	if(max_throughput[context] < tput_kbps)				//updating max throughput if required
		max_throughput[context] = tput_kbps;
}

// A function that accepts all the required values, assigns them to corresponding attributes and returns an ns3 TCP socket
Ptr<Socket> Create_Socket(Address sinkAddress, 
					uint sinkPort, 
					std::string tcpVariant, 
					Ptr<Node> hostNode, 
					Ptr<Node> sinkNode, 
					double startTime, 
					double stopTime,
					uint packetSize,
					uint numPackets,
					std::string dataRate,
					double appStartTime,
					double appStopTime) {

	
		int variant=0;
		//assigning variant type
		if(tcpVariant.compare("TcpHybla") == 0) {
			variant=1;
		}
		else if(tcpVariant.compare("TcpWestwood+") == 0) {
			variant=2;
		}
		else if(tcpVariant.compare("TcpYeah") == 0) {
			variant=3;
		}
		switch(variant){
			case 1:
				Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
				break;
			case 2:
				Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
				break;
			case 3:
				Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpYeah::GetTypeId()));
				break;
			default:
				fprintf(stderr, "Invalid TCP version\n");
				exit(EXIT_FAILURE);
		}
	PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install(sinkNode);
	sinkApps.Start(Seconds(startTime));
	sinkApps.Stop(Seconds(stopTime));

	Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(hostNode, TcpSocketFactory::GetTypeId());
	
	//creating the APP
	Ptr<APP> app = CreateObject<APP>();
	app->Setup(ns3TcpSocket, sinkAddress, packetSize, numPackets, DataRate(dataRate));
	hostNode->AddApplication(app);
	app->SetStartTime(Seconds(appStartTime));
	app->SetStopTime(Seconds(appStopTime));

	return ns3TcpSocket;
}





//Function to initialize and configure the complete network of nodes
void Initialize(Ipv4InterfaceContainer* Receiver_Interfaces1,NodeContainer* Senders1,NodeContainer* Receivers1,int* Packet_Size1){ 
				//all variables passed by reference to reflect back changes
	
	//Given data to be used
	std::string HR_Bandwidth = "100Mbps";
	std::string HR_Delay = "20ms";
	std::string RR_Bandwidth = "10Mbps";
	std::string RR_Delay = "50ms";

	uint Packet_Size = 1.3*KB;		
	uint HR_QueueSize = (2000000)/Packet_Size;  //Number of packets = Queue size= Bandwidth Delay Product / Packet size
	uint RR_QueueSize = (500000)/Packet_Size;
	std::cout<<"Queue Size for Host to Router links: "<<(std::to_string(HR_QueueSize)+" Packets")<<std::endl;
	std::cout<<"Queue Size for Router to Router link: "<<(std::to_string(RR_QueueSize)+" Packets")<<std::endl<<std::endl;
	
	double error = 0.000001;

	//Configuring the network
	PointToPointHelper p2pHR = help_configP2P(HR_Bandwidth, HR_Delay, std::to_string(HR_QueueSize)+"p" );
	PointToPointHelper p2pRR = help_configP2P(RR_Bandwidth, RR_Delay, std::to_string(RR_QueueSize)+"p" );
	Ptr<RateErrorModel> Error_Model = CreateObjectWithAttributes<RateErrorModel> ("ErrorRate", DoubleValue (error));

	//Initialising node containers
	std::cout << "Initialising node containers..."<< std::endl;
	NodeContainer Routers, Senders, Receivers;

	//Create n nodes and append pointers to them to the end of this NodeContainer. 
	Routers.Create(2);
	Senders.Create(num_sender);
	Receivers.Create(num_sender);

	//Initialising NetDeviceContatiners
	NetDeviceContainer routerDevices = p2pRR.Install(Routers);
	
	NetDeviceContainer leftRouterDevices, rightRouterDevices, senderDevices, ReceiverDevices;

	//Adding links
	std::cout << "Adding and configuring links between nodes..."<< std::endl;

	int j=0;
	while(j<num_sender){
		NetDeviceContainer Config_Left = p2pHR.Install(Routers.Get(0), Senders.Get(j)); 
		leftRouterDevices.Add(Config_Left.Get(0)); // left router
		senderDevices.Add(Config_Left.Get(1)); // all devices on left of dumbbell
		Config_Left.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(Error_Model));
		j++;
	
	}
	j=0;
	while(j<num_sender){
		NetDeviceContainer Config_Right = p2pHR.Install(Routers.Get(1), Receivers.Get(j)); 
		rightRouterDevices.Add(Config_Right.Get(0)); // right router
		ReceiverDevices.Add(Config_Right.Get(1)); // all devices on right of dumbbell
		Config_Right.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(Error_Model));
		j++;
	
	}
	
	

	
	//Installing Internet Stack
	std::cout << "Installing internet stack on the nodes..."<< std::endl;
	InternetStackHelper stack;
	stack.Install(Routers);
	stack.Install(Senders);
	stack.Install(Receivers);

	//Assigning IP addresses to the nodes
	std::cout << "Assigning IP addresses to the nodes and initialising network interfaces..."<< std::endl;
	Ipv4AddressHelper routerIP = Ipv4AddressHelper("15.3.0.0", "255.255.255.0");	//(network IP, mask)
	Ipv4AddressHelper senderIP = Ipv4AddressHelper("15.1.0.0", "255.255.255.0");
	Ipv4AddressHelper ReceiverIP = Ipv4AddressHelper("15.2.0.0", "255.255.255.0");

	Ipv4InterfaceContainer router_Interface, sender_Interfaces, Receiver_Interfaces, leftRouter_Interfaces, rightRouter_Interfaces;

	router_Interface = routerIP.Assign(routerDevices);

	int i=0;
	while(i<num_sender) 
	{
		NetDeviceContainer senderDevice;
		senderDevice.Add(senderDevices.Get(i));
		senderDevice.Add(leftRouterDevices.Get(i));

		Ipv4InterfaceContainer sender_Interface = senderIP.Assign(senderDevice);
		sender_Interfaces.Add(sender_Interface.Get(0));
		leftRouter_Interfaces.Add(sender_Interface.Get(1));
		senderIP.NewNetwork();

		NetDeviceContainer ReceiverDevice;
		ReceiverDevice.Add(ReceiverDevices.Get(i));
		ReceiverDevice.Add(rightRouterDevices.Get(i));
		
		Ipv4InterfaceContainer Receiver_Interface = ReceiverIP.Assign(ReceiverDevice);
		Receiver_Interfaces.Add(Receiver_Interface.Get(0));
		rightRouter_Interfaces.Add(Receiver_Interface.Get(1));
		ReceiverIP.NewNetwork();
		i++;
	}

	std::cout<<"finished with initialisation of network \n"<< std::endl<<std::endl;

	// Returning the values back after initialisation
	*Receiver_Interfaces1=Receiver_Interfaces;
	*Senders1=Senders;
	*Receivers1=Receivers;
	*Packet_Size1=Packet_Size;

}

void Create_files(int i,Ptr<OutputStreamWrapper> * tput,
	Ptr<OutputStreamWrapper> * cwnd,
	Ptr<OutputStreamWrapper> * closs,
	Ptr<OutputStreamWrapper> * gput,
	int type)
{	AsciiTraceHelper asciiTraceHelper;
	std:: string filename=types[i];
	if(type==1)
		filename= filename+"_trace_a";
	else
		filename= filename+"_trace_b";
	cwnd[i] = asciiTraceHelper.CreateFileStream(filename+".cw");  //file to store congestion window data for i-th network type
	closs[i] = asciiTraceHelper.CreateFileStream(filename+".cl"); //file to store congestion loss data for i-th network type
	tput[i] = asciiTraceHelper.CreateFileStream(filename+".tp");  //file to store throughput data for i-th network type
	gput[i] = asciiTraceHelper.CreateFileStream(filename+".gp");  //file to store goodput data for i-th network type
}

void statsStorage(std::map<FlowId, FlowMonitor::FlowStats> stats,
	Ptr<OutputStreamWrapper> *closs,
	Ptr<Ipv4FlowClassifier> classifier)
{		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
	int type=-1;
	if(t.sourceAddress == "15.1.0.1") {
		type=0;} 
	else if(t.sourceAddress == "15.1.1.1") {
		type=1;} 
	else if(t.sourceAddress == "15.1.2.1") {
		type=2;}
	
	switch(type){
	
		case 0: 
		*closs[type]->GetStream()<<"** TCP Hybla Flow **\n"<<"Flow ID :"<<i->first<<"\n";	
		*closs[type]->GetStream()<<"Maximum throughput: "<<max_throughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"]<<std::endl;
		break;
		case 1: 
		*closs[type]->GetStream()<<"** TCP Westwood+ Flow **\n"<<"Flow ID :"<<i->first<<"\n";
		*closs[type]->GetStream()<<"Maximum throughput: "<<max_throughput["/NodeList/6/$ns3::Ipv4L3Protocol/Rx"]<<std::endl;
		break;
		case 2:
		*closs[type]->GetStream()<<"** TCP YeAH-TCP Flow **\n"<<"Flow ID :"<<i->first<<"\n";
		*closs[type]->GetStream()<<"Maximum throughput: "<<max_throughput["/NodeList/7/$ns3::Ipv4L3Protocol/Rx"]<<std::endl;
		break;
	
	
	}
	
	
	if(type>=0){
		if(dropped_packets.find(type+1)==dropped_packets.end())
			dropped_packets[type+1] = 0;
		*closs[type]->GetStream()<<"Source IP: "<< t.sourceAddress << " -> Destination IP: "<<t.destinationAddress<<std::endl;
		*closs[type]->GetStream()<<"Total number of packets transmitted: "<<Num_Packets<<std::endl;
		*closs[type]->GetStream()<<"Total number of packets lost: "<<i->second.lostPackets<<std::endl;
		*closs[type]->GetStream()<<"Number of packets transferred successfully: "<<Num_Packets-(i->second.lostPackets)<<std::endl;
		*closs[type]->GetStream()<<"Number of packets lost due to buffer overflow: "<< dropped_packets[type+1]<<std::endl;
		*closs[type]->GetStream()<<"Number of packets lost due to congestion: "<<i->second.lostPackets - dropped_packets[type+1]<<std::endl;
		*closs[type]->GetStream()<<"% loss due to buffer overflow: "<<(dropped_packets[type+1]*100.0)/Num_Packets<<std::endl;
		*closs[type]->GetStream()<<"% loss due to congestion: "<<((i->second.lostPackets - dropped_packets[type+1])*100.0)/Num_Packets<<std::endl;

	}
}
}







void multiFlow()
{
	
	Ipv4InterfaceContainer Receiver_Interfaces;
	NodeContainer Senders, Receivers;
	int Packet_Size;

	Initialize(&Receiver_Interfaces,&Senders,&Receivers,&Packet_Size);

	double Duration_Gap = 100;
	double FirstFlowStart = 0;  //first flow starts at time 0
	double OtherFlowStart = 20; //time when other flows start, while first one in progress
	uint port = 9000;
	std::string Transfer_Speed  = "400Mbps";
	
	// arrays to store output streams for Throughput, Congestion Window, Congestion Loss, Goodput data respectively for each TCP variant
	Ptr<OutputStreamWrapper> *tput=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *cwnd=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *closs=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *gput=new Ptr<OutputStreamWrapper>[num_sender];

	// array to store ns3 TCP sockets for each TCP variant
	Ptr<Socket>* ns3TCPSocket = new Ptr<Socket>[num_sender];

	//array to store sink values for Goodput
	std:: string * sink1= new std::string[num_sender];
	sink1[0]="/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[1]="/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[2]="/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";
	
	//array to store sink values for Throughput
	std:: string * sink2= new std::string[num_sender];
	sink2[0]="/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	sink2[1]="/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	sink2[2]="/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	

	for(int i=0;i<num_sender;i++){
		std::cout<<"From H"<<i<<" to H"<<i+4<<" : ";
		std::cout<<"Connection type: "<<types[i]<<std::endl;
		Create_files(i,tput,cwnd,closs,gput,2); //create files to store data

		// assigning attributes to ns3 TCP sockets
		if(i==0) //First flow starts at FirstFlowStart=0
			ns3TCPSocket[i] = Create_Socket(InetSocketAddress(Receiver_Interfaces.GetAddress(i), port), port,types[i], Senders.Get(i), Receivers.Get(i), FirstFlowStart, FirstFlowStart+Duration_Gap, Packet_Size, Num_Packets, Transfer_Speed , FirstFlowStart, FirstFlowStart+Duration_Gap);
		else     //Other flows start at FirstFlowStart+OtherFlowStart = 20, while first one in progress
			ns3TCPSocket[i] = Create_Socket(InetSocketAddress(Receiver_Interfaces.GetAddress(i), port), port,types[i], Senders.Get(i), Receivers.Get(i), OtherFlowStart, OtherFlowStart+Duration_Gap, Packet_Size, Num_Packets, Transfer_Speed , OtherFlowStart, OtherFlowStart+Duration_Gap);
		
		ns3TCPSocket[i]->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&congestWindowSize, cwnd[i], 0));
		ns3TCPSocket[i]->TraceConnectWithoutContext("Drop", MakeBoundCallback (&dropPackets, closs[i], 0, i+1));
		
		// Measuring packet sinks
		Config::Connect(sink1[i], MakeBoundCallback(&goodput_calculate, gput[i], 0));
		Config::Connect(sink2[i], MakeBoundCallback(&throughput_calculate, tput[i], 0));

	}

	std::cout<<std::endl;

	std::cout<<"Populating Routing Tables"<< std::endl;
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// Enabling IP flow monitoring on all the nodes 
	std::cout<<"Setting up FlowMonitor to enable IP flow monitoring on all the nodes..."<<std::endl;
	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(OtherFlowStart+Duration_Gap));

	// Simulation starts
	std::cout<<"Simulation starting!"<< std::endl;
	Simulator::Run();
	
	std::cout<<"Checking for lost packets"<< std::endl;
	flowmon->CheckForLostPackets();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

	// Retrieving all collected the flow statistics
	std::cout<<"Collecting flow statistics"<< std::endl;
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	statsStorage(stats,closs,classifier);

	Simulator::Destroy();
	std::cout << "Simulation finished!  " << std::endl; 
	return;
}










void singleFlow() 
{
	

	Ipv4InterfaceContainer Receiver_Interfaces;
	NodeContainer Senders, Receivers;
	int Packet_Size;

	//Initialising the network
	Initialize(&Receiver_Interfaces,&Senders,&Receivers,&Packet_Size);

	//Initializing required variables
	double Duration_Gap = 100;
	double Net_Duration = 0;
	uint port = 2500;
	std::string Transfer_Speed  = "400Mbps";	

	// arrays to store output streams for Throughput, Congestion Window, Congestion Loss, Goodput data respectively for each TCP variant
	Ptr<OutputStreamWrapper> *tput=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *cwnd=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *closs=new Ptr<OutputStreamWrapper>[num_sender];
	Ptr<OutputStreamWrapper> *gput=new Ptr<OutputStreamWrapper>[num_sender];

	// array to store ns3 TCP sockets for each TCP variant
	Ptr<Socket>* ns3TCPSocket = new Ptr<Socket>[num_sender];

	//array to store sink values for Goodput
	std:: string * sink1= new std::string[num_sender];
	sink1[0]="/NodeList/5/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[1]="/NodeList/6/ApplicationList/0/$ns3::PacketSink/Rx";
	sink1[2]="/NodeList/7/ApplicationList/0/$ns3::PacketSink/Rx";

	//array to store sink values for Throughput
	std:: string * sink2= new std::string[num_sender];
	sink2[0]="/NodeList/5/$ns3::Ipv4L3Protocol/Rx";
	sink2[1]="/NodeList/6/$ns3::Ipv4L3Protocol/Rx";
	sink2[2]="/NodeList/7/$ns3::Ipv4L3Protocol/Rx";
	

	//Measuring Performance of each TCP variant
	std::cout << "Measuring Performances" << std::endl;

	int i=0;
	do{
		if(num_sender==0) break;
		std::cout<<"From H"<<i+1<<" to H"<<i+4<<" ::: ";;
		std::cout<<"Connection type: "<<types[i]<<std::endl;
		Create_files(i,tput,cwnd,closs,gput,1);   //create files to store data

		// assigning attributes to ns3 TCP sockets
		ns3TCPSocket[i] = Create_Socket(InetSocketAddress(Receiver_Interfaces.GetAddress(i), port), port,types[i], Senders.Get(i), Receivers.Get(i), Net_Duration, Net_Duration+Duration_Gap, Packet_Size, Num_Packets, Transfer_Speed , Net_Duration, Net_Duration+Duration_Gap);
		ns3TCPSocket[i]->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback (&congestWindowSize, cwnd[i], Net_Duration));
		ns3TCPSocket[i]->TraceConnectWithoutContext("Drop", MakeBoundCallback (&dropPackets, closs[i], Net_Duration, i+1));
		
		// Measuring packet sinks
		Config::Connect(sink1[i], MakeBoundCallback(&goodput_calculate, gput[i], Net_Duration));
		Config::Connect(sink2[i], MakeBoundCallback(&throughput_calculate, tput[i], Net_Duration));

		Net_Duration += Duration_Gap; //update net duration elapsed
		i++;
	}while(i<num_sender);

	
	std::cout<<"Measuring performances finished"<<std::endl<<std::endl;

	//Building a routing database and initializing the routing tables of the nodes in the simulation
	std::cout<<"Populating Routing Tables"<< std::endl;
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	// Enabling IP flow monitoring on all the nodes 
	std::cout<<"Setting up FlowMonitor to enable IP flow monitoring on all the nodes"<<std::endl;
	Ptr<FlowMonitor> flowmon;
	FlowMonitorHelper flowmonHelper;
	flowmon = flowmonHelper.InstallAll();
	Simulator::Stop(Seconds(Net_Duration));

	// Simulation starts
	std::cout<<"Simulation starting!"<< std::endl;
	Simulator::Run();
	
	std::cout<<"Checking for lost packets"<< std::endl;
	flowmon->CheckForLostPackets();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

	// Retrieving all collected the flow statistics
	std::cout<<"Collecting flow statistics"<< std::endl;
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
	statsStorage(stats,closs,classifier); //storing flow statistics in file

	Simulator::Destroy();
	std::cout << "Simulation finished! " << std::endl; 
	return;
}



