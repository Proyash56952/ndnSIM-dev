from ns.core import *
from ns.network import *
from ns.mobility import MobilityHelper, ListPositionAllocator
from ns.ndnSIM import ndn
from ns.internet import InternetStackHelper, Ipv4StaticRoutingHelper, Ipv4

from ns.lte import LteHelper, PointToPointEpcHelper, LteSidelinkHelper, LteSpectrumValueHelper, LteSlUeRrc, LteRrcSap, LteSlPreconfigPoolFactory, LteSlTft
# custom helper
from ns.lte import addLteSlPool

import sys, os
import ns.applications
import time

if 'SUMO_HOME' in os.environ:
    tools = os.path.join(os.environ['SUMO_HOME'], 'tools')
    sys.path.append(tools)
else:
    sys.exit("please declare environment variable 'SUMO_HOME'")

import traci
import traci.constants as tc
import time
import re
import sumolib

net = sumolib.net.readNet('src/ndnSIM/scenarios/sumo/intersection.net.xml')

def runSumo(t):
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
        sumoCmd = ["sumo", "-c", "src/ndnSIM/scenarios/sumo/intersection.sumocfg", "--load-state", "state_"+str(t-1)+".xml", "--begin", str(t-1)]
    else:
        sumoCmd = ["sumo", "-c", "src/ndnSIM/scenarios/sumo/intersection.sumocfg"]
        #d1 = addNodeToContainer("f1.0")
        #d2 = addNodeToContainer("f3.0")
        #d3 = addNodeToContainer("f4.0")
        #d4 = addNodeToContainer("f5.0")
    traci.start(sumoCmd)
    traci.simulationStep(t)
    vehicles=traci.vehicle.getIDList()
    persons = traci.person.getIDList()
    
    for j in range(0,len(persons)):
        perPos = traci.person.getPosition(persons[j])
        perSpeed = traci.person.getSpeed(persons[j])
        print("position of person" + str(persons[j]) + " is: " + str(perPos) + " and speed is: "+ str(perSpeed))
    
    for i in range(0,len(vehicles)):
        speed = round(traci.vehicle.getSpeed(vehicles[i]))
        pos = traci.vehicle.getPosition(vehicles[i])
        x = round(pos[0],1)
        y = round(pos[1],1)
        angle = traci.vehicle.getAngle(vehicles[i])

        f.write('$ns_ at '+str(t)+' "$node_('+str(vehicles[i])+') setdest '+str(x)+' '+str(y)+' '+str(speed)+' '+str(angle)+'"\n')
        
        
        # get the lane id in which the vehicle is currently on
        lane = traci.vehicle.getLaneID(vehicles[i])
        #print("lane: "+lane+" "+vehicles[i])
        
        if(lane == "1i_3"):
        #get the lane object using sumolib.net
            lane_ = net.getLane(lane)
            
            # retrieve the list of outgoing lanes from current lane
            outGoing = lane_.getOutgoing()
            
            if(len(outGoing) > 0):
                nextLaneId = outGoing[0].getToLane().getID() #get the next lane the vehilce will visit
                #laneLength = outGoing[0].getToLane().getLength()
                print("next lane will be : "+nextLaneId)
                #print("length: "+str(laneLength))
            

            x,y = sumolib.geomhelper.positionAtShapeOffset(net.getLane(nextLaneId).getShape(), 0)
            print("The position will be : " +str(x)+"  " +str(y))
            
    f.close()

    ns2 = ns.ndnSIM.CustomHelper("ns2-traceFile"+str(t)+".tcl")
    ns2.Install()
    #nodeIds = ns2.getNodeNames();
    #print(ueNodes.GetN())
    if(t==0):
        print("number of nodes: "+str(ueNodes.GetN()))
        for u in range (0,ueNodes.GetN()):
            print("the node is: "+str(ueNodes.Get(u)))
            setHelpers(ueNodes.Get(u))

    traci.simulation.saveState("state_"+str(t)+".xml")
    traci.close()



# Configure the UE for UE_SELECTED scenario
Config.SetDefault("ns3::LteUeMac::SlGrantMcs", UintegerValue(16))
Config.SetDefault("ns3::LteUeMac::SlGrantSize", UintegerValue(5)) # The number of RBs allocated per UE for Sidelink
Config.SetDefault("ns3::LteUeMac::Ktrp", UintegerValue(1))
Config.SetDefault("ns3::LteUeMac::UseSetTrp", BooleanValue(True)) # use default Trp index of 0

# Set error models
Config.SetDefault("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue(True))
Config.SetDefault("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue(True))
Config.SetDefault("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue(False))

# Set the UEs power in dBm
Config.SetDefault ("ns3::LteUePhy::TxPower", DoubleValue(23.0))

cmd = CommandLine()

cmd.traceFile = "intersecMobility.tcl"
cmd.sumo_granularity = Seconds(1)
cmd.duration = Seconds(5)
cmd.logFile = "default.log"
cmd.tmin = 0.02
cmd.tmax = 0.2
cmd.vis = False

cmd.AddValue("traceFile", "Name of the Trace File")
cmd.AddValue("duration", "Total simulation time")
cmd.AddValue("logFile", "Name of the log File")
cmd.AddValue("tmin", "minimum time")
cmd.AddValue("tmax", "maximum time")
cmd.AddValue("sumo_granularity", "Granularity of SUMO")
cmd.AddValue("vis", "enable visualizer")

cmd.Parse(sys.argv)

if cmd.vis:
    GlobalValue.Bind("SimulatorImplementationType", StringValue("ns3::VisualSimulatorImpl"))

# Sidelink bearers activation time
slBearersActivationTime = Seconds(2.0)

lteHelper = LteHelper()
epcHelper = PointToPointEpcHelper()
lteHelper.SetEpcHelper(epcHelper)

# Create Sidelink helper and set lteHelper
proseHelper = LteSidelinkHelper()
proseHelper.SetLteHelper(lteHelper)

# Enable Sidelink
lteHelper.SetAttribute("UseSidelink", BooleanValue(True))

# Set pathloss model
lteHelper.SetAttribute("PathlossModel", StringValue("ns3::Cost231PropagationLossModel"))
lteHelper.SetPathlossModelAttribute("BSAntennaHeight", DoubleValue(1.5))
lteHelper.SetPathlossModelAttribute("SSAntennaHeight", DoubleValue(1.5))
# channel model initialization
lteHelper.Initialize()

# Set the frequency
ulEarfcn = 18100
ulBandwidth = 75

# Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
ulFreq = LteSpectrumValueHelper.GetCarrierFrequency(ulEarfcn)

uplinkPathlossModel = lteHelper.GetUplinkPathlossModel()
uplinkPathlossModel.SetAttributeFailSafe("Frequency", DoubleValue(ulFreq))

# Sidelink pre-configuration for the UEs
ueSidelinkConfiguration = LteSlUeRrc()
ueSidelinkConfiguration.SetSlEnabled(True)

preconfiguration = LteRrcSap.SlPreconfiguration()

preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn
preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth

pfactory = LteSlPreconfigPoolFactory()

# Control
pfactory.SetControlPeriod("sf40")
pfactory.SetControlBitmap(0x00000000FF) # 8 subframes for PSCCH
pfactory.SetControlOffset(0)
pfactory.SetControlPrbNum(22)
pfactory.SetControlPrbStart(0)
pfactory.SetControlPrbEnd(49)

# Data
pfactory.SetDataBitmap(0xFFFFFFFFFF)
pfactory.SetDataOffset(8) # After 8 subframes of PSCCH
pfactory.SetDataPrbNum(25)
pfactory.SetDataPrbStart(0)
pfactory.SetDataPrbEnd(49)

# preconfiguration.preconfigComm.nbPools = 1
addLteSlPool(preconfiguration, pfactory.CreatePool())

ueSidelinkConfiguration.SetSlPreconfiguration(preconfiguration)

internet = InternetStackHelper()
groupL2Address = 255;
groupAddress4 = Ipv4Address("225.63.63.1")

remoteAddress = InetSocketAddress(groupAddress4, 8000)
localAddress = InetSocketAddress(Ipv4Address.GetAny(), 8000)
tft = LteSlTft(LteSlTft.BIDIRECTIONAL, groupAddress4, groupL2Address)

## Check which mobility model we should setup
#mobility = MobilityHelper()
# initialAlloc = ListPositionAllocator()
# initialAlloc.Add(Vector(0.0,   0.0, 0.0))
# initialAlloc.Add(Vector(700,   0.0, 0.0))
# initialAlloc.Add(Vector(100.0, 0.0, 0.0))
# mobility.SetPositionAllocator(initialAlloc)
#mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel")

ipv4RoutingHelper = Ipv4StaticRoutingHelper()
ndnHelper = ndn.StackHelper()
ndnHelper.SetDefaultRoutes(True)

ueNodes = ns.network.NodeContainer()
#ueNodes.Create(4)
# ///*** Configure applications ***///
def addNode(name):
    node = Node()
    print ("adding the node named: "+str(node))
    mobility.Install(node)
    ueDev = lteHelper.InstallUeDevice(NodeContainer(node))

    lteHelper.InstallSidelinkConfiguration(ueDev, ueSidelinkConfiguration)
    internet.Install(node)

    ueIpIface = epcHelper.AssignUeIpv4Address(NetDeviceContainer(ueDev))

    ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(node.GetObject(Ipv4.GetTypeId()))
    ueStaticRouting.SetDefaultRoute(epcHelper.GetUeDefaultGatewayAddress(), 1)

    proseHelper.ActivateSidelinkBearer(slBearersActivationTime, ueDev, tft)

    Names.Add(name, node)

    ndnHelper.Install(node)

    # Choosing forwarding strategy
    ndn.StrategyChoiceHelper.Install(node, "/", "/localhost/nfd/strategy/directed-geocast/%FD%01/" +str(cmd.tmin) + "/" + str(cmd.tmax))

    class Tuple:
        def __init__(self, node, dev, ip):
            self.node = node
            self.dev = dev
            self.ip = ip
    return Tuple(node, ueDev.Get(0), ueIpIface.GetAddress(0, 0))

def addNodeToContainer(name):
    node = Node()
    print("add node: "+str(node.GetId())+" and is at: "+str(node))
    ueNodes.Add(node)
    Names.Add(name, node)
    return node


def setHelpers(node):
    print("node is is: "+str(node.GetId()))
    print("setHelpers")
    ueDev = lteHelper.InstallUeDevice(NodeContainer(node))

    lteHelper.InstallSidelinkConfiguration(ueDev, ueSidelinkConfiguration)
    internet.Install(node)

    ueIpIface = epcHelper.AssignUeIpv4Address(NetDeviceContainer(ueDev))

    ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(node.GetObject(Ipv4.GetTypeId()))
    ueStaticRouting.SetDefaultRoute(epcHelper.GetUeDefaultGatewayAddress(), 1)

    proseHelper.ActivateSidelinkBearer(slBearersActivationTime, ueDev, tft)

    #Names.Add(name, node)

    ndnHelper.Install(node)

    # Choosing forwarding strategy
    ndn.StrategyChoiceHelper.Install(node, "/", "/localhost/nfd/strategy/directed-geocast/%FD%01/" +str(cmd.tmin) + "/" + str(cmd.tmax))
    print("reaches the end")
    
# #In this loop, we call an event runSumo() for each second. runSumo will use tracy to simulate a sumo scenario, and extract output for a particular second and write those into a trace file. Finally, using our customized mobility helper, mobility will be installed in nodes.
for i in range(0,20):
    ns.core.Simulator.Schedule(ns.core.Seconds(i), runSumo, i)
    #time.sleep(0.5)

#c1 = addNodeToContainer("car1")
#c2 = addNodeToContainer("car2")

#ueNodes = ns.network.NodeContainer()
#ueNodes.Create(4)

#consumerHelper = ndn.AppHelper("ns3::ndn::ConsumerCbr")
#consumerHelper.SetPrefix("/v2safety/8thStreet/0,0,0/700,0,0/100")
#consumerHelper.SetAttribute("Frequency", StringValue("1"))
#consumerHelper.Install(c1.node)

#producerHelper = ndn.AppHelper("ns3::ndn::Producer")
#producerHelper.SetPrefix("/v2safety/8thStreet")
#producerHelper.SetAttribute("PayloadSize", StringValue("50"))
#producerHelper.Install(c2.node)

Simulator.Stop(cmd.duration)
Simulator.Run()

