import ns.applications
import ns.internet
import ns.mobility
import ns.config_store
import ns.wifi
import ns.olsr

from ns.core import *
from ns.network import *
from ns.point_to_point import *
from ns.ndnSIM import *

import sys, os

import traci
import traci.constants as tc
import time
import re

#This function is being called on each second. It will run sumo scenario, extract data for a particular second and writes those into a trace file. Then using our customized helper, we install mobility to the nodes according to the trace file.
def runSumo0(nodes,t):
    #print("hola")
    if 'SUMO_HOME' in os.environ:
        tools = os.path.join(os.environ['SUMO_HOME'], 'tools')
        sys.path.append(tools)
    else:
        sys.exit("please declare environment variable 'SUMO_HOME'")

    f = open("ns2-traceFile"+str(t)+".tcl","w+")
    sumoCmd = ["sumo", "-c", "src/ndnSIM/sumo/intersection/intersection.sumocfg", "--start"]
    traci.start(sumoCmd)
    traci.simulationStep(t)
    vehicles=traci.vehicle.getIDList();
    for i in range(0,len(vehicles)):
        speed = round(traci.vehicle.getSpeed(vehicles[i]))
        pos = traci.vehicle.getPosition(vehicles[i])
        x = round(pos[0],1)
        y = round(pos[1],1)
        angle = traci.vehicle.getAngle(vehicles[i])

        f.write('$ns_ at '+str(t)+' "$node_('+str(i)+') setdest '+str(x)+' '+str(y)+' '+str(speed)+' '+str(angle)+'"\n')
    
    f.close()

    ns2 = ns.ndnSIM.CustomHelper("ns2-traceFile"+str(t)+".tcl")
    ns2.Install()
    
    traci.simulation.saveState("fileName.txt")
    traci.close()

def runSumo(nodes,t):
    #print("hola")
    if 'SUMO_HOME' in os.environ:
        tools = os.path.join(os.environ['SUMO_HOME'], 'tools')
        sys.path.append(tools)
    else:
        sys.exit("please declare environment variable 'SUMO_HOME'")

    f = open("ns2-traceFile"+str(t)+".tcl","w+")
    #sumoCmd = ["sumo", "-c", "src/ndnSIM/sumo/intersection/intersection.sumocfg", "--load-state", "fileName.xml", "--begin", "2"]
    #traci.start(sumoCmd)
    if(t > 0):
        sumoCmd = ["sumo", "-c", "src/ndnSIM/sumo/intersection/intersection.sumocfg", "--load-state", "state_"+str(t-1)+".xml", "--begin", str(t-1)]
    else:
        sumoCmd = ["sumo", "-c", "src/ndnSIM/sumo/intersection/intersection.sumocfg"]
    traci.start(sumoCmd)
    traci.simulationStep(t)
    vehicles=traci.vehicle.getIDList();
    for i in range(0,len(vehicles)):
        speed = round(traci.vehicle.getSpeed(vehicles[i]))
        pos = traci.vehicle.getPosition(vehicles[i])
        x = round(pos[0],1)
        y = round(pos[1],1)
        angle = traci.vehicle.getAngle(vehicles[i])

        f.write('$ns_ at '+str(t)+' "$node_('+str(i)+') setdest '+str(x)+' '+str(y)+' '+str(speed)+' '+str(angle)+'"\n')

    f.close()

    ns2 = ns.ndnSIM.CustomHelper("ns2-traceFile"+str(t)+".tcl")
    ns2.Install()

    traci.simulation.saveState("state_"+str(t)+".xml")
    traci.close()
    
    
cmd = ns.core.CommandLine()
cmd.nodeNum = 4
cmd.traceFile = "intersecMobility.tcl"
cmd.duration = 20
cmd.logFile = "default.log"
cmd.tmin = 0.02
cmd.tmax = 0.2
cmd.AddValue("nodeNum", "Number of nodes")
cmd.AddValue("traceFile", "Name of the Trace File")
cmd.AddValue("duration", "Total simulation time")
cmd.AddValue("logFile", "Name of the log File")
cmd.AddValue("tmin", "minimum time")
cmd.AddValue("tmax", "maximum time")

cmd.Parse(sys.argv)

nodeNum = int(cmd.nodeNum)
duration = float(cmd.duration)
tmin = float(cmd.tmin)
tmax = float(cmd.tmax)
traceFile = cmd.traceFile

wifiNodes = ns.network.NodeContainer()
wifiNodes.Create(nodeNum)

wifi = ns.wifi.WifiHelper()
mac = ns.wifi.WifiMacHelper()
mac.SetType("ns3::AdhocWifiMac")
wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                  "DataMode", ns.core.StringValue("OfdmRate54Mbps"))

wifiPhy = ns.wifi.YansWifiPhyHelper.Default()
wifiChannel = ns.wifi.YansWifiChannelHelper.Default()
wifiPhy.SetChannel(wifiChannel.Create())
wifiDevices = wifi.Install(wifiPhy, mac, wifiNodes)
    # 
    #  Add the IPv4 protocol stack to the nodes in our container
    # 

internet = ns.internet.InternetStackHelper()
internet.Install(wifiNodes);

#  Assign IPv4 addresses to the device drivers(actually to the associated
#  IPv4 interfaces) we just created.

ipAddrs = ns.internet.Ipv4AddressHelper()
ipAddrs.SetBase(ns.network.Ipv4Address("192.168.0.0"), ns.network.Ipv4Mask("255.255.255.0"))
ipAddrs.Assign(wifiDevices)

#mobility = ns.mobility.MobilityHelper()
#mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                 # "MinX", ns.core.DoubleValue(20.0),
                                 # "MinY", ns.core.DoubleValue(20.0),
                                 # "DeltaX", ns.core.DoubleValue(20.0),
                                 # "DeltaY", ns.core.DoubleValue(20.0),
                                 # "GridWidth", ns.core.UintegerValue(5),
                                  #"LayoutType", ns.core.StringValue("RowFirst"))
#mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel",
                               #"Bounds", ns.mobility.RectangleValue(ns.mobility.Rectangle(-500, 500, -500, 500)),
                             #  "Speed", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                              # "Pause", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"))
#mobility.Install(wifiNodes)

#ns2 = ns.ndnSIM.CustomHelper(traceFile)
#ns2.Install()

#In this loop, we call an event runSumo() for each second. runSumo will use tracy to simulate a sumo scenario, and extract output for a particular second and write those into a trace file. Finally, using our customized mobility helper, mobility will be installed in nodes.
for i in range(0,int(duration)):
    ns.core.Simulator.Schedule(ns.core.Seconds(i), runSumo, wifiNodes,i)

ndnHelper = ndn.StackHelper()
ndnHelper.SetDefaultRoutes(True)
ndnHelper.InstallAll()

# Choosing forwarding strategy
ndn.StrategyChoiceHelper.InstallAll("/", "/localhost/nfd/strategy/directed-geocast/%FD%01/" +str(tmin) + "/" + str(tmax))

consumerHelper = ndn.AppHelper("ns3::ndn::ConsumerCbr")
consumerHelper.SetPrefix("/v2safety/8thStreet/0,0,0/700,0,0/100")
consumerHelper.SetAttribute("Frequency", StringValue("1"))
consumerHelper.Install(wifiNodes.Get(0))

producerHelper = ndn.AppHelper("ns3::ndn::Producer")
producerHelper.SetPrefix("/v2safety/8thStreet")
producerHelper.SetAttribute("PayloadSize", StringValue("50"))
producerHelper.Install(wifiNodes.Get(nodeNum-1))

Simulator.Stop(Seconds(duration))
Simulator.Run()
