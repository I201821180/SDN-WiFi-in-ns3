/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Peking University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: XieYingying <xyynku@163.com>
 *
 * a SDN-based Wi-Fi network with 2 APs and 1 STA
 * STA is handoffed from AP1 to AP2
 */
#include <iomanip>
#include <string>
#include <unordered_map>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/internet-apps-module.h>
#include <ns3/wifi-module.h>
#include <ns3/spectrum-module.h>
#include "ns3/mobility-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LoadBalanceTest");
static const uint32_t packetSize = 1420;
class NodeStatistics
{
public:
	NodeStatistics ();

	void CheckStatistics (double time);
	void RxCallback (std::string path, Ptr<const Packet> packet, const Address &from);
	Gnuplot2dDataset GetDatafile ();

private:
	uint32_t m_bytesTotal;
	Gnuplot2dDataset m_output;
};

NodeStatistics::NodeStatistics ()
{
	m_bytesTotal = 0;
	m_output.SetTitle ("Throughput Mbits/s");
}

void
NodeStatistics::RxCallback (std::string path, Ptr<const Packet> packet, const Address &from)
{
	m_bytesTotal += packet->GetSize ();
}

void
NodeStatistics::CheckStatistics (double time)
{
	std::cout << "CheckStatictics:" << Simulator::Now() <<std::endl;
	double mbs = ((m_bytesTotal * 8.0) / (1000000 * time));
	m_bytesTotal = 0;
	m_output.Add ((Simulator::Now ()).GetSeconds (), mbs);
	Simulator::Schedule (Seconds (time), &NodeStatistics::CheckStatistics, this, time);
}

Gnuplot2dDataset
NodeStatistics::GetDatafile ()
{
	return m_output;
}


int
main (int argc, char *argv[])
{
	double simTime = 3;        //Seconds
	bool verbose = true;
	bool trace = true;
	std::string errorModelType = "ns3::NistErrorRateModel";
	double distance = 20;        //meters
	std::string outputFileName = "throughput";

	// Configure command line parameters
	CommandLine cmd;
	cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
	cmd.AddValue ("verbose", "Enable verbose output", verbose);
	cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
	cmd.AddValue ("errorModelType", "select ns3::NistErrorRateModel or ns3::YansErrorRateModel", errorModelType);
	cmd.AddValue ("distance", "distance between nodes", distance);
	cmd.Parse (argc, argv);

	if (verbose)
    {
		OFSwitch13Helper::EnableDatapathLogs ();
		//LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
		//LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
		//LogComponentEnable ("WifiNetDevice", LOG_LEVEL_ALL);
		//LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_ALL);
		//LogComponentEnable ("Simulator", LOG_LEVEL_ALL);
		LogComponentEnable ("OFSwitch13WifiController", LOG_LEVEL_ALL);
		LogComponentEnable ("WifiElements", LOG_LEVEL_ALL);
		//LogComponentEnable ("WifiPhy", LOG_LEVEL_ALL);
		//LogComponentEnable ("SpectrumWifiPhy", LOG_LEVEL_ALL);
		//LogComponentEnable ("UdpServer", LOG_LEVEL_ALL);
		//LogComponentEnable ("UdpClient", LOG_LEVEL_ALL);
	    //LogComponentEnable ("PropagationLossModel", LOG_LEVEL_ALL);
		//LogComponentEnable ("ApWifiMac", LOG_LEVEL_ALL);
		//LogComponentEnable ("RegularWifiMac", LOG_LEVEL_ALL);
		//LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
		//LogComponentEnable ("MacLow", LOG_LEVEL_ALL);
		//LogComponentEnable ("WifiRemoteStationManager", LOG_LEVEL_ALL);
		LogComponentEnable ("LoadBalanceTest", LOG_LEVEL_ALL);
    }
	

	// Enable checksum computations (required by OFSwitch13 module)
	GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

	// Create two AP nodes
	NodeContainer aps;
	aps.Create (2);
	NodeContainer stas;
	stas.Create (5);
	
	// Create the switch node
	Ptr<Node> switchNode = CreateObject<Node> ();

	// Use the CsmaHelper to connect AP nodes to the switch node
	CsmaHelper csmaHelper;
	csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("15Mbps")));
	//csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0.1)));
	NetDeviceContainer apDevices;
	NetDeviceContainer switchPorts;
	
	NodeContainer pair1 (aps.Get (0), switchNode);
	NetDeviceContainer link1 = csmaHelper.Install (pair1);
	apDevices.Add (link1.Get (0));
	switchPorts.Add (link1.Get (1));
	
	csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate("10Mbps")));
	NodeContainer pair2 (aps.Get(1), switchNode);
	NetDeviceContainer link2 = csmaHelper.Install (pair2);
	apDevices.Add (link2.Get (0));
	switchPorts.Add (link2.Get (1));
    	
	Ptr<Node> host = CreateObject<Node> ();
	NetDeviceContainer hostDevices;
	NodeContainer tmp (switchNode, host);
	NetDeviceContainer link = csmaHelper.Install(tmp);
	switchPorts.Add(link.Get(0));
	hostDevices.Add(link.Get(1));
	
	Config::SetDefault ("ns3::WifiPhy::CcaMode1Threshold", DoubleValue (-62.0));
    
	// spectrum channel configuration
	Ptr<MultiModelSpectrumChannel> channel
		= CreateObject<MultiModelSpectrumChannel> ();
	Ptr<FriisPropagationLossModel> lossModel
		= CreateObject<FriisPropagationLossModel> ();
	//lossModel->SetFrequency (5.180e9);
	channel->AddPropagationLossModel (lossModel);
	Ptr<ConstantSpeedPropagationDelayModel> delayModel
		= CreateObject<ConstantSpeedPropagationDelayModel> ();
	channel->SetPropagationDelayModel (delayModel);
	
	// spectrum phy configuration
	SpectrumWifiPhyHelper phy = SpectrumWifiPhyHelper::Default ();
	phy.SetChannel (channel);
	phy.SetErrorRateModel (errorModelType);
	phy.Set ("TxPowerStart", DoubleValue (100)); // dBm  (1.26 mW)
	phy.Set ("TxPowerEnd", DoubleValue (100));
	//phy.Set ("ShortGuardEnabled", BooleanValue (false));
	phy.Set ("ChannelWidth", UintegerValue (160));
	phy.Set ("GuardInterval", TimeValue (NanoSeconds (800)));
	
	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue ("HeMcs11"),
								  "ControlMode", StringValue ("HeMcs11"));
	
	//mac configuration
	WifiMacHelper wifiMac;
	NetDeviceContainer apWifiDevs;
	NetDeviceContainer staWifiDevs;
	
	Ssid ssid = Ssid ("wifi1");
	wifiMac.SetType ("ns3::ApWifiMac",
					 "Ssid", SsidValue (ssid),
					 "EnableBeaconJitter", BooleanValue (false));
	apWifiDevs.Add(wifi.Install (phy, wifiMac, aps));
	wifiMac.SetType ("ns3::StaWifiMac",
					 "ActiveProbing", BooleanValue (true),
					 "Ssid", SsidValue (ssid));
	staWifiDevs.Add (wifi.Install (phy, wifiMac, stas));
	
	//mobility configuration
	MobilityHelper mobility;
	
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (distance, distance, 0.0));
	positionAlloc->Add (Vector (distance*3, distance, 0.0));
	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	positionAlloc->Add (Vector (distance, 0.0, 0.0));
	positionAlloc->Add (Vector (distance*1.5, 0.0, 0.0));
	positionAlloc->Add (Vector (distance*3, 0.0, 0.0));
	positionAlloc->Add (Vector (distance*4, 0.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (aps);
	mobility.Install (stas);

	// Create the controller node
	Ptr<Node> controllerNode = CreateObject<Node> ();

	// Configure the OpenFlow network domain
	Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
	Ptr<OFSwitch13WifiController> wifiControl = CreateObject<OFSwitch13WifiController> ();
	of13Helper->InstallController (controllerNode, wifiControl);
	of13Helper->InstallSwitch (switchNode, switchPorts);
	for (size_t i = 0; i < aps.GetN(); i++)
	{
		NetDeviceContainer tmp;
		tmp.Add (apDevices.Get (i));
		tmp.Add (apWifiDevs.Get(i));
		of13Helper->InstallSwitch (aps.Get (i), tmp);
	}
	of13Helper->CreateOpenFlowChannels ();
	
	// Install the TCP/IP stack into STA nodes
	InternetStackHelper internet;
	internet.Install (stas);
	internet.Install (host);

	// Set IPv4 addresses for STAs
	Ipv4AddressHelper ipv4helpr;
	ipv4helpr.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer staIpIfaces;
	staIpIfaces = ipv4helpr.Assign (staWifiDevs);
	Ipv4InterfaceContainer hostIpIfaces;
	hostIpIfaces = ipv4helpr.Assign (hostDevices);
	std::set<Ipv4Address> staIpv4;
	for (uint32_t i = 0 ; i < staIpIfaces.GetN(); ++i)
	{
		staIpv4.insert (staIpIfaces.GetAddress(i));
	}
	//Configure the CBR generator
	uint16_t port = 9;
	PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (hostIpIfaces.GetAddress(0), port));
	ApplicationContainer apps_sink = sink.Install (host);
	apps_sink.Start (Seconds (1));
	apps_sink.Stop (Seconds (simTime + 1));	
	OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (hostIpIfaces.GetAddress(0), port));
	onoff.SetConstantRate (DataRate ("20Mb/s"), packetSize);
	onoff.SetAttribute ("StartTime", TimeValue (Seconds (1)));
	onoff.SetAttribute ("StopTime", TimeValue (Seconds (simTime+1)));
	onoff.Install (stas.Get(0));
	onoff.Install (stas.Get(1));
	//onoff.Install (stas.Get(2));
	onoff.SetConstantRate (DataRate("10Mb/s"), packetSize);
	onoff.Install (stas.Get(3));
	//onoff.Install (stas.Get(4));
						   	
	// Enable datapath stats and pcap traces at APs and controller(s)
	if (trace)
    {
		of13Helper->EnableOpenFlowPcap ("openflow");
		//of13Helper->EnableDatapathStats ("ap-openflow-stats");
		phy.EnablePcap ("apWifi", apWifiDevs.Get(0));
		//phy.EnablePcap ("staWifi", staWifiDevs);
		//csmaHelper.EnablePcap ("host", hostDevices);
    }
	
	//Statistics counters
	NodeStatistics statistics = NodeStatistics ();

	//Register packet receptions to calculate throughput
	Config::Connect ("/NodeList/"+std::to_string(host->GetId())+"/ApplicationList/*/$ns3::PacketSink/Rx",
					 MakeCallback (&NodeStatistics::RxCallback, &statistics));
	
	statistics.CheckStatistics (1);

   	Simulator::Schedule (Seconds (3), &OFSwitch13WifiController::ChannelQualityReportStrategy,
						 wifiControl);
	Simulator::Schedule(Seconds(5), &OFSwitch13WifiController::PrintAssocStatus,
						wifiControl);
	Simulator::Schedule(Seconds(6), &OFSwitch13WifiController::PrintChannelquality,
						wifiControl);
	//Simulator::Schedule (Seconds(11), &OFSwitch13WifiController::ConfigAssocLBStrategy,
	//					 wifiControl);
	//Simulator::Schedule(Seconds(15), &OFSwitch13WifiController::PrintAssocStatus,
	//				     wifiControl);
	Simulator::Stop (Seconds (simTime + 2));
	
	// calculate per STA throughput
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
	
	// Run the simulation
	Simulator::Run ();
	NS_LOG_INFO ("*******Address Info************");
	for (uint32_t i = 0 ; i < apWifiDevs.GetN(); ++i)
	{
		NS_LOG_INFO("AP(" << i << "):" << DynamicCast<WifiNetDevice>(apWifiDevs.Get(i))->GetMac()->GetAddress());
	}
	for (uint32_t i = 0 ; i < stas.GetN(); ++i)
	{
		NS_LOG_INFO("STA(" << i << "):Mac" << DynamicCast<WifiNetDevice>(staWifiDevs.Get(i))->GetMac()->GetAddress() << 
					";IP:" << staIpIfaces.GetAddress(i));
	}
	
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		if (staIpv4.find((Ipv4Address(t.sourceAddress))) != staIpv4.end() && Address(t.destinationAddress) == hostIpIfaces.GetAddress(0))
        {
			NS_LOG_INFO ("Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n");
			NS_LOG_INFO ("  Tx Bytes:   " << i->second.txBytes << "\n");
			NS_LOG_INFO ("  Rx Bytes:   " << i->second.rxBytes << "\n");
			NS_LOG_UNCOND ("  Throughput of " << t.sourceAddress << ":"  << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024  << " Mbps\n");
			NS_LOG_INFO ("  Mean delay:   " << i->second.delaySum.GetSeconds () / i->second.rxPackets << "\n");
			NS_LOG_INFO ("  Mean jitter:   " << i->second.jitterSum.GetSeconds () / (i->second.rxPackets - 1) << "\n");
        }
    }
	
	//Plots
	std::ofstream outfileTh0 (("throughput-" + outputFileName + "-0.plt").c_str ());
	Gnuplot gnuplot = Gnuplot (("throughput-" + outputFileName + "-0.eps").c_str (), "Throughput");
	gnuplot.SetTerminal ("post eps color enhanced");
	gnuplot.SetLegend ("Time (seconds)", "Throughput (Mb/s)");
	gnuplot.SetTitle ("Throughput (STA to host) vs time");
	gnuplot.AddDataset (statistics.GetDatafile ());
	gnuplot.GenerateOutput (outfileTh0);
	
	Simulator::Destroy ();
	return 0;
}
