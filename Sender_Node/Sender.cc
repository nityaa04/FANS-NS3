 /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
3  * Copyright (c) 2012 University of Washington, 2012 INRIA
4  *
5  * This program is free software; you can redistribute it and/or modify
6  * it under the terms of the GNU General Public License version 2 as
7  * published by the Free Software Foundation;
8  *
9  * This program is distributed in the hope that it will be useful,
10  * but WITHOUT ANY WARRANTY; without even the implied warranty of
11  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
12  * GNU General Public License for more details.
13  *
14  * You should have received a copy of the GNU General Public License
15  * along with this program; if not, write to the Free Software
16  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
17  *
18  * Author: Alina Quereilhac <alina.quereilhac@inria.fr>
19  *
20  */
 
 // +----------------------+     +-----------------------+
 // |      client host     |     |      server host      |
 // +----------------------+     +-----------------------+
 // |     ns-3 Node 0      |     |      ns-3 Node 1      |
 // |  +----------------+  |     |   +----------------+  |
 // |  |    ns-3 TCP    |  |     |   |    ns-3 TCP    |  |
 // |  +----------------+  |     |   +----------------+  |
 // |  |    ns-3 IPv4   |  |     |   |    ns-3 IPv4   |  |
 // |  +----------------+  |     |   +----------------+  |
 // |  |   FdNetDevice  |  |     |   |   FdNetDevice  |  |
 // |  |    10.1.1.1    |  |     |   |    10.1.1.2    |  |
 // |  +----------------+  |     |   +----------------+  |
 // |  |   raw socket   |  |     |   |   raw socket   |  |
 // |  +----------------+  |     |   +----------------+  |
 // |       | eth0 |       |     |        | eth0 |       |
 // +-------+------+-------+     +--------+------+-------+
 //
//         10.1.1.11                     10.1.1.12
 //
 //             |                            |
 //             +----------------------------+
 //
 // This example is aimed at measuring the throughput of the FdNetDevice
 // when using the EmuFdNetDeviceHelper. This is achieved by saturating
 // the channel with TCP traffic. Then the throughput can be obtained from 
 // the generated .pcap files.
 //
 // To run this example you will need two hosts (client & server).
 // Steps to run the experiment:
 //
 // 1 - Connect the 2 computers with an Ethernet cable.
 // 2 - Set the IP addresses on both Ethernet devices.
 //
 // client machine: $ sudo ip addr add dev eth0 10.1.1.11/24
 // server machine: $ sudo ip addr add dev eth0 10.1.1.12/24
 //
 // 3 - Set both Ethernet devices to promiscuous mode.
 //
// both machines: $ sudo ip link set eth0 promisc on
 //
 // 4 - Give root suid to the raw socket creator binary.
 //     If the --enable-sudo option was used to configure ns-3 with waf, then the following
 //     step will not be necessary.
 //
 // both hosts: $ sudo chown root.root build/src/fd-net-device/ns3-dev-raw-sock-creator
 // both hosts: $ sudo chmod 4755 build/src/fd-net-device/ns3-dev-raw-sock-creator
 //
 // 5 - Run the server side:
 //
 // server host: $ ./waf --run="fd-emu-onoff --serverMode=1"
 //
 // 6 - Run the client side:
 //       
 // client host: $ ./waf --run="fd-emu-onoff"
 //
 
 #include <iostream>
 #include <fstream>
 #include <vector>
 #include <string>
 
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/config-store-module.h"
 #include "ns3/fd-net-device-module.h"
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE ("EmuFdNetDeviceSaturationExample");

 int x= 0;
 std::string msg = std::to_string(x);
 //uint8_t *buffer=&x;
 //uint32_t buffersize=1;
 //uint32_t flags;
 static void
 GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval)
 {
    if (pktCount > 0)
    {   
        //Create<Packet>(pktSize);
        //std::cout<<"Sending data uav0 "<<*buffer<<std::endl;
        ns3::Ptr<ns3::Packet> packet = ns3::Create<ns3::Packet>((uint8_t*)::msg.c_str(),::msg.length());        
        socket->Send(packet);
        ::x = ::x +1;
        ::msg = std::to_string(::x);
        std::cout<<"Sending data uav0: "<<::msg<<std::endl;
        Simulator::Schedule(pktInterval,
                            &GenerateTraffic,
                            socket,
                            pktSize,
                            pktCount - 1,
                            pktInterval);
    }
    else
    {
        socket->Close();
    }
 }

 
 int 
 main (int argc, char *argv[])
 {
   uint16_t sinkPort = 80;
   uint32_t packetSize = 1000; // bytes
   uint32_t pktcount = 50;
   double interval = 1.0;
   Time interPacketInterval = Seconds(interval);
   std::string dataRate("1000Mb/s");
 
   std::string deviceName ("wlan0");
   std::string client ("10.1.1.1");
   std::string server ("10.1.1.3");
   std::string netmask ("255.255.255.0");
   std::string macClient ("e8:4e:06:32:25:07");
   std::string macServer ("e8:4e:06:32:28:dd");
 
   Ipv4Address remoteIp;
   Ipv4Address localIp;
   Mac48AddressValue localMac;
   
   remoteIp = Ipv4Address (server.c_str ());
   localIp = Ipv4Address (client.c_str ());
   localMac =  Mac48AddressValue (macClient.c_str ());
 
   Ipv4Mask localMask (netmask.c_str ());
   
   GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
 
   GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
 
   NS_LOG_INFO ("Create Node");
   Ptr<Node> node = CreateObject<Node> ();
 
   NS_LOG_INFO ("Create Device");
   EmuFdNetDeviceHelper emu;
   emu.SetDeviceName (deviceName);
   NetDeviceContainer devices = emu.Install (node);
   Ptr<NetDevice> device = devices.Get (0);
   device->SetAttribute ("Address", localMac);
 
   NS_LOG_INFO ("Add Internet Stack");
   InternetStackHelper internetStackHelper;
   internetStackHelper.SetIpv4StackInstall(true);
   internetStackHelper.Install (node);
 
   NS_LOG_INFO ("Create IPv4 Interface");
   Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
   uint32_t interface = ipv4->AddInterface (device);
   Ipv4InterfaceAddress address = Ipv4InterfaceAddress (localIp, localMask);
   ipv4->AddAddress (interface, address);
   ipv4->SetMetric (interface, 1);
   ipv4->SetUp (interface);

   //TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");  
   TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
   Ptr<Socket> source = Socket::CreateSocket(node, tid);
   InetSocketAddress remote = InetSocketAddress(remoteIp, sinkPort);
   source->Connect(remote);

   Simulator::Schedule(Seconds(5.0),
                        &GenerateTraffic,
                        source,
                        packetSize,
                        pktcount,
                        interPacketInterval);

 
   emu.EnablePcap ("3nt-source", device);
 
   Simulator::Stop (Seconds (600.0));
   Simulator::Run ();
   Simulator::Destroy ();
 
   return 0;
 }
