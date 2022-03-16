/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
.*/

/*
 * This is a simple example to test different tcp cc algorithm in a incast scenerio.
 * Network topology, modified based on provided bridge example:
 *  K incast scenario
 *
 *  K * node -> ethernet switch -> destination node
 *  topology graph:
 *
 *                 node0
 *                  |
 *                  |
 * ---switch(fixed bandwidth)---------
 * |   |   |                        |
 * n1  n2  n3 . . . . . . . . . . . nK
 *
 * each node 1~K send as much traffic as possible to node 0
 * trying to fill the bandwidth.
 *
 */

#include <iostream>
#include <fstream>
#include <queue>

#include "ns3/error-model.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("test-incast-example");
double queue_stat = 0;
double queue_stat_num = 0;
std::priority_queue<double> rtt_pq;
bool redir_output = true;

// referring to congestion_control example
void TcPacketsInQueueTrace(uint32_t oldValue, uint32_t newValue)
{
    queue_stat += newValue;
    queue_stat_num += 1;

    if (redir_output)
    {
        std::cout << "Q," << newValue << "," << ns3::Simulator::Now().GetMicroSeconds() << std::endl;
    }
}
// tail/avg rtt stat
void TCPrtt_stat(double rtt)
{
    if (redir_output)
    {
        std::cout << "RTT," << rtt << "," << ns3::Simulator::Now().GetMicroSeconds() << std::endl;
    }
    rtt_pq.push(rtt);
}

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::FS);

    // algorithm parameters to be set

    //parameter for timely with incast 10 and 25Mbps
    double Alpha = 0.6;
    double AI = 1.0;
    double MD = 0.05;
    double Hth = 4500;
    double Lth = 500;
    double initial_rate = 5;
    uint32_t n = 5; // HAI


    // test env parameters
    std::string bandWidth = "25Mbps";
    std::string delay = "2us";
    double start_t = 1;
    double end_t = 11;
    int active_incast = 15;
    int incast_num = 15;
    redir_output = true;
    bool show_options = false;
    int queueSize = 800000; // bytes

    std::string congestion_rule = "TCPCCTIMELY";

    CommandLine ops;
    ops.AddValue("congestion", "Protocol to use: TCPCCTIMELY, TcpNewReno, TcpLinuxReno, "
                               "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                               "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
                               "TcpLp, TcpDctcp, TcpCubic",
                 congestion_rule);
    ops.AddValue("h", "Congestion control rule", show_options);
    ops.AddValue("bandwitdh", "egress bandwidth", bandWidth);
    ops.AddValue("incast", "incast num", incast_num);
    ops.AddValue("incast", "active incast", active_incast);
    ops.AddValue("Alpha", "Alpha", Alpha);
    ops.AddValue("AI", "AI", AI);
    ops.AddValue("MD", "MD", MD);
    ops.AddValue("initial_rate", "initial_rate", initial_rate);
    ops.AddValue("Hth", "Hth", Hth);
    ops.AddValue("Lth", "Lth", Lth);
    ops.AddValue("HAI", "HAI counter", n);

    Config::SetDefault("ns3::TcpCongestionOps::TCP_RTTstat", CallbackValue(MakeCallback(&TCPrtt_stat)));

    // running flag
    ops.Parse(argc, argv);
    if (show_options)
    {
        std::cout << "-------------------options-----------------" << std::endl;
        std::cout << "running format: ./waf --run \"scratch/test --option_name=option_value ...\"" << std::endl;
        std::cout << "--h [just showing manu: true/false, default false]" << std::endl;
        std::cout << "--congestion [congestion options: Protocol to use: TCPCCTIMELY, TcpNewReno, TcpLinuxReno, "
                     "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                     "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
                     "TcpLp, TcpDctcp, TcpCubic, default TIMELY]"
                  << std::endl;
        std::cout << "--bandwitdh [define bandwidth: for example, 25Mbps]" << std::endl;
        std::cout << "--incast [define incast size: for example, 10]" << std::endl;
        std::cout << "--Alpha [define EWMA weight: for example, 0.5]" << std::endl;
        std::cout << "--AI [define additive increase param: for example, 1]" << std::endl;
        std::cout << "--MD [define multiplicative decrease param: for example, 0.05]" << std::endl;
        std::cout << "--Lth [define Lower threshold: for example, 500]" << std::endl;
        std::cout << "--Hth [define Higher threshold: for example, 5500]" << std::endl;
        std::cout << "--initial_rate [define initial sending rate: for example, 5]" << std::endl;
        std::cout << "--HAI [define HAI counter: for example, 5]" << std::endl;


        return 0;
    }

    // support tcp congestion rules
    if (congestion_rule.compare("TCPCCTIMELY") == 0)
    {
        std::cout << "~~~Using tcp TIMELY ~~~" << std::endl;
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TCPCCTIMELY::GetTypeId()));
        Config::SetDefault("ns3::TCPCCTIMELY::Alpha", DoubleValue(Alpha));
        Config::SetDefault("ns3::TCPCCTIMELY::AI", DoubleValue(AI));
        Config::SetDefault("ns3::TCPCCTIMELY::MD", DoubleValue(MD));
        Config::SetDefault("ns3::TCPCCTIMELY::Hth", DoubleValue(Hth));
        Config::SetDefault("ns3::TCPCCTIMELY::Lth", DoubleValue(Lth));
        Config::SetDefault("ns3::TCPCCTIMELY::N_hai", UintegerValue(n));
        Config::SetDefault("ns3::TCPCCTIMELY::initial_rate", DoubleValue(initial_rate));
        Config::SetDefault("ns3::TcpSocketBase::ClockGranularity", TimeValue(Time("1ns")));
    }
    else
    {
        // other tcp congestion control methods, all use default parameters
        TypeId tid;
        // NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(congestion_rule, &tid), "TypeId " << congestion_rule << " not found");
        std::cout << "Running with " << congestion_rule << std::endl;
        congestion_rule = std::string("ns3::") + congestion_rule;
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName(congestion_rule)));
    }

    NodeContainer nodeContainer;
    // N to 1 incast
    nodeContainer.Create(incast_num + 1);

    // one ethernet switch
    NodeContainer ethernetSwitch;
    ethernetSwitch.Create(1);
    CsmaHelper helper;
    // set bandwidth and delay
    helper.SetChannelAttribute("DataRate", StringValue(bandWidth)); 
    helper.SetChannelAttribute("Delay", StringValue(delay));       

    NetDeviceContainer nodedevicecontainer;
    NetDeviceContainer switchdevicecontainer;

    for (int i = 0; i < incast_num + 1; i++)
    {
        NetDeviceContainer connection = helper.Install(NodeContainer(nodeContainer.Get(i), ethernetSwitch));
        nodedevicecontainer.Add(connection.Get(0));
        switchdevicecontainer.Add(connection.Get(1));
    }
    Ptr<Node> ethernetSwitchPtr = ethernetSwitch.Get(0);
    BridgeHelper br;
    br.Install(ethernetSwitchPtr, switchdevicecontainer);

    // install L4 stack
    InternetStackHelper internet;
    internet.Install(nodeContainer);

    Ipv4AddressHelper ip;
    ip.SetBase("11.11.11.0", "255.255.255.0");
    ip.Assign(nodedevicecontainer);

    NS_LOG_INFO("Create Applications.");
    uint16_t tcp_port = 8080;

    // packet sinker to node index 0
    // sinker
    PacketSinkHelper pktsinkhelper("ns3::TcpSocketFactory",
                                   Address(InetSocketAddress(Ipv4Address::GetAny(), tcp_port)));
    ApplicationContainer appSink = pktsinkhelper.Install(nodeContainer.Get(0));
    appSink.Start(Seconds(start_t));

    //  destination host
    AddressValue remoteaddr(InetSocketAddress(Ipv4Address("11.11.11.1"), tcp_port));

    // sender
    ApplicationContainer srcapp;
    int incast = 0;

    for (int i = 1; i < incast_num + 1 && active_incast > 0; i++)
    {

        std::string src_ip = "11.11.11.";
        src_ip = src_ip + std::to_string(1 + i);

        BulkSendHelper source_helper("ns3::TcpSocketFactory",
                                     InetSocketAddress(Ipv4Address(src_ip.c_str()),
                                                       7070 + i));
        source_helper.SetAttribute("MaxBytes", UintegerValue(0));
        source_helper.SetAttribute("Remote", remoteaddr);
        srcapp.Add(source_helper.Install(nodeContainer.Get(i)));
        active_incast -= 1;
        incast += 1;
    }
    srcapp.Start(Seconds(start_t));
    srcapp.Stop(Seconds(end_t));

    std::cout << "Configured incast nodes number: " << std::to_string(incast) << std::endl;

    NS_LOG_INFO("Tracing:");

    Ptr<Queue<Packet>> switch_queue = DynamicCast<CsmaNetDevice>(switchdevicecontainer.Get(0))->GetQueue();

    switch_queue->SetMaxSize(QueueSize(BYTES, queueSize));
    switch_queue->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(&TcPacketsInQueueTrace));

    AsciiTraceHelper tracer_helper;
    helper.EnableAsciiAll(tracer_helper.CreateFileStream("tracing_data.txt"));
    helper.EnablePcapAll("csma-bridge", false);

    NS_LOG_INFO("Run Simulation.");
    // Simulator::Stop(Seconds(end_t+10));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    double totalrtt = 0;
    int size = (int)(rtt_pq.size() * 0.01);
    int median = (int)(rtt_pq.size() * 0.5);
    int len = rtt_pq.size();
    int i = 0;
    for (; i < size; i++)
    {
        totalrtt += rtt_pq.top();
        // pop largest one
        rtt_pq.pop();
    }
    double rtt_99 = rtt_pq.top();
    for (; i < median; i++)
    {
        totalrtt += rtt_pq.top();
        rtt_pq.pop();
    }
    double rtt_50 = rtt_pq.top();

    for (; i < len; i++)
    {
        totalrtt += rtt_pq.top();
        rtt_pq.pop();
    }
    std::cout << "99-percentile RTT: " << rtt_99 << " μs" << std::endl;
    std::cout << "Median RTT: " << rtt_50 << " μs" << std::endl;
    std::cout << "Average RTT: " << totalrtt / len << " μs" << std::endl;
    std::cout << "AVG queue occupancy: " << queue_stat / queue_stat_num << " pkts" << std::endl;
    double avg_throughput = DynamicCast<PacketSink>(appSink.Get(0))->GetTotalRx() * 8.0 / (end_t - start_t) / 1000000;
    std::cout << "AVG Throughput: " << avg_throughput << "Mbps" << std::endl;
}
