from ns.core import *
from ns.network import *
from ns.mobility import MobilityHelper, ListPositionAllocator
from ns.ndnSIM import ndn
from ns.internet import InternetStackHelper, Ipv4StaticRoutingHelper, Ipv4

from ns.lte import LteHelper, PointToPointEpcHelper, LteSidelinkHelper, LteSpectrumValueHelper, LteSlUeRrc, LteRrcSap, LteSlPreconfigPoolFactory, LteSlTft
# custom helper
from ns.lte import addLteSlPool

import sys, os

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
cmd.duration = Seconds(20)
cmd.logFile = "default.log"
cmd.tmin = 0.02
cmd.tmax = 0.2

cmd.AddValue("traceFile", "Name of the Trace File")
cmd.AddValue("duration", "Total simulation time")
cmd.AddValue("logFile", "Name of the log File")
cmd.AddValue("tmin", "minimum time")
cmd.AddValue("tmax", "maximum time")
cmd.AddValue("sumo_granularity", "Granularity of SUMO")

cmd.Parse(sys.argv)

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

print(preconfiguration.preconfigComm.nbPools)

ueSidelinkConfiguration.SetSlPreconfiguration(preconfiguration)

internet = InternetStackHelper()
groupL2Address = 255;
groupAddress4 = Ipv4Address("225.63.63.1")

remoteAddress = InetSocketAddress(groupAddress4, 8000)
localAddress = InetSocketAddress(Ipv4Address.GetAny(), 8000)
tft = LteSlTft(LteSlTft.BIDIRECTIONAL, groupAddress4, groupL2Address)

## Check which mobility model we should setup
mobility = MobilityHelper()
# initialAlloc = ListPositionAllocator()
# initialAlloc.Add(Vector(0.0,   0.0, 0.0))
# initialAlloc.Add(Vector(700,   0.0, 0.0))
# initialAlloc.Add(Vector(100.0, 0.0, 0.0))
# mobility.SetPositionAllocator(initialAlloc)
mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel")

ipv4RoutingHelper = Ipv4StaticRoutingHelper()
ndnHelper = ndn.StackHelper()
ndnHelper.SetDefaultRoutes(True)

# ///*** Configure applications ***///
def addNode(name):
    node = Node()
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

    return node, ueDev.Get(0), ueIpIface.GetAddress(0, 0)

c1 = addNode("car1")
c2 = addNode("car2")
c3 = addNode("car3")

def printNode(node):
    print("Node", node[0].GetId(), "(" + Names.FindName(node[0]) + ")", node[1], str(node[2]))

printNode(c1)
printNode(c2)
printNode(c3)

# #In this loop, we call an event runSumo() for each second. runSumo will use tracy to simulate a sumo scenario, and extract output for a particular second and write those into a trace file. Finally, using our customized mobility helper, mobility will be installed in nodes.
# for i in range(0,int(duration)):
#     ns.core.Simulator.Schedule(ns.core.Seconds(i), runSumo, wifiNodes,i)

# consumerHelper = ndn.AppHelper("ns3::ndn::ConsumerCbr")
# consumerHelper.SetPrefix("/v2safety/8thStreet/0,0,0/700,0,0/100")
# consumerHelper.SetAttribute("Frequency", StringValue("1"))
# consumerHelper.Install(wifiNodes.Get(0))

# producerHelper = ndn.AppHelper("ns3::ndn::Producer")
# producerHelper.SetPrefix("/v2safety/8thStreet")
# producerHelper.SetAttribute("PayloadSize", StringValue("50"))
# producerHelper.Install(wifiNodes.Get(nodeNum-1))

Simulator.Stop(cmd.duration)
Simulator.Run()
