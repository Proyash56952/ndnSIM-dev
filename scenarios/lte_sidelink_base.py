from ns.core import *
from ns.network import *
from ns.mobility import MobilityHelper, ListPositionAllocator
from ns.ndnSIM import ndn, CustomHelper
from ns.internet import InternetStackHelper, Ipv4StaticRoutingHelper, Ipv4

from ns.lte import LteHelper, PointToPointEpcHelper, LteSidelinkHelper, LteSpectrumValueHelper, LteSlUeRrc, LteRrcSap, LteSlPreconfigPoolFactory, LteSlTft
# custom helper
from ns.lte import addLteSlPool

import sys, os

cmd = CommandLine()

cmd.traceFile = "intersecMobility.tcl"
cmd.sumo_granularity = Seconds(1)
cmd.duration = Seconds(20)
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

if isinstance(cmd.duration, str):
    cmd.duration = Seconds(float(cmd.duration))

if isinstance(cmd.sumo_granularity, str):
    cmd.sumo_granularity = Seconds(float(cmd.sumo_granularity))

## Check which mobility model we should setup
mobility = MobilityHelper()
mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel")

ndnHelper = ndn.StackHelper()
ndnHelper.SetDefaultRoutes(True)

customHelper = CustomHelper()

# ///*** Configure applications ***///
def addNode(name):
    node = Node()
    mobility.Install(node)
    customHelper.Install(node) # install LTE SideLink + Internet

    Names.Add(name, node)

    ndnHelper.Install(node)

    # Choosing forwarding strategy
    ndn.StrategyChoiceHelper.Install(node, "/", "/localhost/nfd/strategy/directed-geocast/%FD%01/" +str(cmd.tmin) + "/" + str(cmd.tmax))

    class Tuple:
        def __init__(self, node, dev, ip, name):
            self.node = node
            self.dev = dev
            self.ip = ip
            self.name = name
    return Tuple(node, None, None, name) # ueDev.Get(0), ueIpIface.GetAddress(0, 0), name)
