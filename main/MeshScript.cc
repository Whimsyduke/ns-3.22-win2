#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/mesh-module.h"
#include "ns3/wifi-phy.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/random-variable.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("MeshScript");

class MeshClass
{
public:
	// Init test
	MeshClass ();
	// Configure test from command line arguments
	void Configure (int argc, char ** argv);
	// Run test
	int Run ();
private:
	int m_xSize; //x size of the grid
	int m_ySize; //y size of the grid
	double m_step; //separation between nodes
	double m_randomStart;
	double m_totalTime;
	uint16_t m_packetSize;
	uint32_t m_nIfaces;
	bool m_chan;
	bool m_pcap;
	std::string m_stack;
	std::string m_txrate;
	//to calculate the lenght of the simulation
	float m_timeTotal, m_timeStart, m_timeEnd;
	// List of network nodes
	NodeContainer nodes;
	// List of all mesh point devices
	NetDeviceContainer meshDevices;
	// Addresses of interfaces:
	Ipv4InterfaceContainer interfaces;
	// MeshHelper. Report is not static methods
	MeshHelper mesh;
private:
	// Create nodes and setup their mobility
	void CreateNodes ();
	// Install internet m_stack on nodes
	void InstallInternetStack ();
	// Install applications randomly
	void InstallApplicationRandom ();
	// Print mesh devices diagnostics
	void Report ();
};
MeshClass::MeshClass () :
	m_xSize (5),
	m_ySize (5),
	m_step (170),
	m_randomStart (0.1),
	m_totalTime (240),
	m_packetSize (1024),
	m_nIfaces (1),
	m_chan (false),
	m_pcap (false),
	m_stack ("ns3::Dot11sStack"),
	m_txrate ("150kbps")
{
}
void
	MeshClass::Configure (int argc, char *argv[])
{
	CommandLine cmd;
	cmd.AddValue ("m_xSize", "m_xSize", m_xSize);
	cmd.AddValue ("m_ySize", "m_ySize", m_ySize);
	cmd.AddValue ("m_txrate", "m_txrate", m_txrate);
	cmd.Parse (argc, argv);
}
void MeshClass::CreateNodes ()
{
	int i, j;
	double m_txpower = 18.0; // dbm
	// Calculate m_ySize*m_xSize stations grid topology
	double position_x = 0;
	double position_y = 0;
	ListPositionAllocator myListPositionAllocator;
	for (i = 1; i <= m_xSize; i++){
		for (j = 1; j <= m_ySize; j++){
			//std::cout << "Node at x = " << position_x << ", y = " << position_y << "\n";
			NS_LOG_INFO("Node at x = " << position_x << ", y = " << position_y << "\n");
			Vector3D n_pos (position_x, position_y, 0.0);
			myListPositionAllocator.Add(n_pos);
			position_y += m_step;
		}
		position_y = 0;
		position_x += m_step;
	}
	// Create the nodes
	nodes.Create (m_xSize*m_ySize);
	// Configure YansWifiChannel
	YansWifiPhyHelper WifiPhy = YansWifiPhyHelper::Default ();
	WifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-89.0) );
	WifiPhy.Set ("CcaMode1Threshold", DoubleValue (-62.0) );
	WifiPhy.Set ("TxGain", DoubleValue (1.0) );
	WifiPhy.Set ("RxGain", DoubleValue (1.0) );
	WifiPhy.Set ("TxPowerLevels", UintegerValue (1) );
	WifiPhy.Set ("TxPowerEnd", DoubleValue (m_txpower) );
	WifiPhy.Set ("TxPowerStart", DoubleValue (m_txpower) );
	WifiPhy.Set ("RxNoiseFigure", DoubleValue (7.0) );
	YansWifiChannelHelper WifiChannel;
	WifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	WifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","Exponent", StringValue ("2.7"));
	WifiPhy.SetChannel (WifiChannel.Create ());
	// Configure the parameters of the Peer Link
	Config::SetDefault ("ns3::dot11s::PeerLink::MaxBeaconLoss", UintegerValue (20));
	Config::SetDefault ("ns3::dot11s::PeerLink::MaxRetries", UintegerValue (4));
	Config::SetDefault ("ns3::dot11s::PeerLink::MaxPacketFailure", UintegerValue (5));
	// Configure the parameters of the Peer Management Protocol
	Config::SetDefault ("ns3::dot11s::PeerManagementProtocol::EnableBeaconCollision		Avoidance",BooleanValue (false));
	// Configure the parameters of the HWMP
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPactivePathTimeout", TimeValue (Seconds (100)));
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPactiveRootTimeout", TimeValue (Seconds (100)));
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPmaxPREQretries", UintegerValue (5));
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::UnicastPreqThreshold", UintegerValue (10));
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::UnicastDataThreshold", UintegerValue (5));
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::DoFlag", BooleanValue (true));
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::RfFlag", BooleanValue (false));
	// Stack installer creates all protocols and install them to mesh point device
	mesh = MeshHelper::Default ();
	mesh.SetStandard (WIFI_PHY_STANDARD_80211a);
	mesh.SetMacType ("RandomStart", TimeValue (Seconds(m_randomStart)));
	mesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (2500));
	// Set number of interfaces - default is single-interface mesh point
	mesh.SetNumberOfInterfaces (m_nIfaces);
	mesh.SetStackInstaller (m_stack);
	//If multiple channels is activated
	if (m_chan) {
		mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
	}
	else {
		mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
	}
	// Install protocols and return container if MeshPointDevices
	meshDevices = mesh.Install (WifiPhy, nodes);
	// Place the protocols in the positions calculated before
	MobilityHelper mobility;
	mobility.SetPositionAllocator(&myListPositionAllocator);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (nodes);
}
void MeshClass::InstallInternetStack ()
{
	//Install the internet protocol stack on all nodes
	InternetStackHelper internetStack;
	internetStack.Install (nodes);
	//Assign IP addresses to the devices interfaces
	Ipv4AddressHelper address;
	address.SetBase ("192.168.1.0", "255.255.255.0");
	interfaces = address.Assign (meshDevices);
}
void MeshClass::InstallApplicationRandom ()
{
	// Create as many connections as nodes has the grid
	int m_nconn = m_xSize * m_ySize;
	int i=0;
	int m_source, m_dest, m_dest_port;
	char num [2];
	char onoff [7];
	char sink [6];
	double start_time, stop_time, duration;
	// Set the parameters of the onoff application
	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (m_packetSize));
	Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (m_txrate));
	//ApplicationContainer apps [m_nconn];
	UniformVariable rand_nodes (0,m_ySize*m_xSize-1);
	UniformVariable rand_port (49000,49100);
	// 50 seconds for transitori are left at the beginning.
	UniformVariable a(50,m_totalTime-15);
	for (i = 0; i < m_nconn; i++){
		ApplicationContainer apps;
		start_time = a.GetValue();
		ExponentialVariable b(30);
		duration = b.GetValue()+1;
		// If the exponential variable gives us a value that added to the start time
		// is greater than the maximum permitted, this is changed for the maximum
		// 10 seconds are left at the end to calculate well the statistics of each flow
		if ( (start_time + duration) > (m_totalTime - 10)){
			stop_time = m_totalTime-10;
		}else{
			stop_time = start_time + duration;
		}
		// Create different names for the connections
		// (we can not use vectors for OnOffHelper)
		strcpy(onoff,"onoff");
		strcpy(sink,"sink");
		sprintf(num,"%d",i);
		strcat(onoff,num);
		strcat(sink,num);
		// Set random variables of the destination (server) and destination port.
		m_dest = rand_nodes.GetInteger (0,m_ySize*m_xSize-1);
		// Set random variables of the source (client)
		m_source = rand_nodes.GetInteger (0,m_ySize*m_xSize-1);
		// Client and server can not be the same node.
		while (m_source == m_dest){
			m_source = rand_nodes.GetInteger (0,m_ySize*m_xSize-1);
		}
		// Plot the connection values
		//std::cout << "\n\t Node "<< m_source << " to " << m_dest;
		//std::cout << "\n Start_time: " << start_time << "s";
		//std::cout << "\n Stop_time: " << stop_time << "s\n";
		NS_LOG_INFO("\n\t Node "<< m_source << " to " << m_dest);
		NS_LOG_INFO("\n Start_time: " << start_time << "s");
		NS_LOG_INFO("\n Stop_time: " << stop_time << "s\n");
		// Define UDP traffic for the onoff application
		OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (interfaces.GetAddress (m_dest), m_dest_port)));
		onoff.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (1)));
		onoff.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
		//apps[i] = onoff.Install (nodes.Get(m_source));
		//apps[i].Start (Seconds (start_time));
		//apps[i].Stop (Seconds (stop_time));
		apps = onoff.Install (nodes.Get(m_source));
		apps.Start (Seconds (start_time));
		apps.Stop (Seconds (stop_time));
		// Create a packet sink to receive the packets
		PacketSinkHelper sink ("ns3::UdpSocketFactory",InetSocketAddress (interfaces.GetAddress (m_dest), 49001));
		//apps[i] = sink.Install (nodes.Get (m_dest));
		//apps[i].Start (Seconds (1.0));
		apps = sink.Install (nodes.Get (m_dest));
		apps.Start (Seconds (1.0));
	}
}
int MeshClass::Run ()
{
	CreateNodes ();
	InstallInternetStack ();
	InstallApplicationRandom ();
	// Install FlowMonitor on all nodes
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
	m_timeStart=clock();
	Simulator::Schedule (Seconds(m_totalTime), & MeshClass::Report, this);
	Simulator::Stop (Seconds (m_totalTime));
	Simulator::Run ();
	// Define variables to calculate the metrics
	int k=0;
	int totaltxPackets = 0;
	int totalrxPackets = 0;
	double totaltxbytes = 0;
	double totalrxbytes = 0;
	double totaldelay = 0;
	double totalrxbitrate = 0;
	double difftx, diffrx;
	double pdf_value, rxbitrate_value, txbitrate_value, delay_value;
	double pdf_total, rxbitrate_total, delay_total;
	//Print per flow statistics
	monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		difftx = i->second.timeLastTxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
		diffrx = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstRxPacket.GetSeconds();
		pdf_value = (double) i->second.rxPackets / (double) i->second.txPackets * 100;
		txbitrate_value = (double) i->second.txBytes * 8 / 1024 / difftx;
		if (i->second.rxPackets != 0){
			rxbitrate_value = (double)i->second.rxPackets * m_packetSize * 8 / 1024 / diffrx;
			delay_value = (double) i->second.delaySum.GetSeconds() / (double) i->second.rxPackets;
		}
		else{
			rxbitrate_value = 0;
			delay_value = 0;
		}
		// We are only interested in the metrics of the data flows
		if ((!t.destinationAddress.IsSubnetDirectedBroadcast("255.255.255.0")))
		{
			k++;
			// Plot the statistics for each data flow
			//std::cout << "\nFlow " << k << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			NS_LOG_INFO("\nFlow " << k << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n");
			//std::cout << "Tx Packets: " << i->second.txPackets << "\n";
			//std::cout << "Rx Packets: " << i->second.rxPackets << "\n";
			//std::cout << "Lost Packets: " << i->second.lostPackets << "\n";
			//std::cout << "Dropped Packets: " << i->second.packetsDropped.size() << "\n";
			//std::cout << "PDF: " << pdf_value << " %\n";
			//std::cout << "Average delay: " << delay_value << "s\n";
			//std::cout << "Rx bitrate: " << rxbitrate_value << " kbps\n";
			//std::cout << "Tx bitrate: " << txbitrate_value << " kbps\n\n";
			NS_LOG_INFO("PDF: " << pdf_value << " %\n");
			NS_LOG_INFO("Tx Packets: " << i->second.txPackets << "\n");
			NS_LOG_INFO("Rx Packets: " << i->second.rxPackets << "\n");
			NS_LOG_INFO("Lost Packets: " << i->second.lostPackets << "\n");
			NS_LOG_INFO("Dropped Packets: " << i->second.packetsDropped.size() << "\n");
			NS_LOG_INFO("PDF: " << pdf_value << " %\n");
			NS_LOG_INFO("Average delay: " << delay_value << "s\n");
			NS_LOG_INFO("Rx bitrate: " << rxbitrate_value << " kbps\n");
			NS_LOG_INFO("Tx bitrate: " << txbitrate_value << " kbps\n\n");
			// Acumulate for average statistics
			totaltxPackets += i->second.txPackets;
			totaltxbytes += i->second.txBytes;
			totalrxPackets += i->second.rxPackets;
			totaldelay += i->second.delaySum.GetSeconds();
			totalrxbitrate += rxbitrate_value;
			totalrxbytes += i->second.rxBytes;
		}
	}
	// Average all nodes statistics
	if (totaltxPackets != 0){
		pdf_total = (double) totalrxPackets / (double) totaltxPackets * 100;
	}
	else{
		pdf_total = 0;
	}
	if (totalrxPackets != 0){
		rxbitrate_total = totalrxbitrate;
		delay_total = (double) totaldelay / (double) totalrxPackets;
	}
	else{
		rxbitrate_total = 0;
		delay_total = 0;
	}
	//print all nodes statistics
	//std::cout << "\nTotal PDF: " << pdf_total << " %\n";
	//std::cout << "Total Rx bitrate: " << rxbitrate_total << " kbps\n";
	//std::cout << "Total Delay: " << delay_total << " s\n";
	NS_LOG_INFO("\nTotal PDF: " << pdf_total << " %\n");
	NS_LOG_INFO("Total Rx bitrate: " << rxbitrate_total << " kbps\n");
	NS_LOG_INFO("Total Delay: " << delay_total << " s\n");
	//print all nodes statistics in files
	std::ostringstream os;
	os << "1_HWMP_PDF.txt";
	//std::ofstream of (os.str().c_str(), ios::out | ios::app);
	std::ofstream of (os.str().c_str());
	of << pdf_total << "\n";
	of.close ();
	std::ostringstream os2;
	os2 << "1_HWMP_Delay.txt";
	//std::ofstream of2 (os2.str().c_str(), ios::out | ios::app);
	std::ofstream of2 (os2.str().c_str());
	of2 << delay_total << "\n";
	of2.close ();
	std::ostringstream os3;
	os3 << "1_HWMP_Throu.txt";
	//std::ofstream of3 (os3.str().c_str(), ios::out | ios::app);
	std::ofstream of3 (os3.str().c_str());
	of3 << rxbitrate_total << "\n";
	of3.close ();
	Simulator::Destroy ();
	m_timeEnd=clock();
	m_timeTotal=(m_timeEnd - m_timeStart)/(double) CLOCKS_PER_SEC;
	//std::cout << "\n*** Simulation time: " << m_timeTotal << "s\n\n";
	NS_LOG_INFO("\n*** Simulation time: " << m_timeTotal << "s\n\n");
	return 0;
}
void MeshClass::Report ()
{
	// Using this function we print detailed statistics of each mesh point device
	// These statistics are used later with an AWK files to extract routing metrics
	unsigned n (0);
	for (NetDeviceContainer::Iterator i = meshDevices.Begin ();
		i != meshDevices.End (); ++i, ++n)
	{
		std::ostringstream os;
		//os << "mp-report1-" << n << ".xml";
		os << "mp-report.xml";
		std::ofstream of;
		//of.open (os.str().c_str(), ios::out | ios::app);
		of.open (os.str().c_str());
		if (! of.is_open ())
		{
			//std::cerr << "Error: Can't open file " << os.str() << "\n";
			NS_LOG_INFO("Error: Can't open file " << os.str() << "\n");
			return;
		}
		mesh.Report (*i, of);
		of.close ();
	}
	n = 0;
}
int main (int argc, char *argv[])
{
	MeshClass t;
	t.Configure (argc, argv);
	return t.Run();
}