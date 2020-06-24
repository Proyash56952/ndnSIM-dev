import ns.applications
import ns.internet
import ns.mobility
import sys
import ns.lte
import ns.config_store
from ns.core import *
from ns.network import *
from ns.point_to_point import *
from ns.ndnSIM import *
#from ns.lte import *


cmd = ns.core.CommandLine()
cmd.nodeNum = 2
cmd.traceFile = "default.tcl"
cmd.duration = 10
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

print(nodeNum)

ueNodes = ns.network.NodeContainer()
ueNodes.Create(nodeNum)

Config.SetDefault("ns3::LteUeMac::SlGrantMcs", UintegerValue (16))
Config.SetDefault("ns3::LteUeMac::SlGrantSize", UintegerValue (5))
Config.SetDefault("ns3::LteUeMac::Ktrp", UintegerValue (1))
Config.SetDefault("ns3::LteUeMac::UseSetTrp", BooleanValue (True))

ulEarfcn = 18100;
ulBandwidth = 75;

Config.SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue(True))
Config.SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue(True))
Config.SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (False))

Config.SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0))
Config.SetDefault ("ns3::LteUePowerControl::Pcmax", DoubleValue (23.0))
Config.SetDefault ("ns3::LteUePowerControl::PscchTxPower", DoubleValue (23.0))

slBearersActivationTime = ns.core.Time(2.0)

lteHelper = ns.lte.LteHelper()

epcHelper = ns.lte.PointToPointEpcHelper()
lteHelper.SetEpcHelper (epcHelper)

proseHelper = ns.lte.LteSidelinkHelper()
proseHelper.SetLteHelper(lteHelper)


lteHelper.SetAttribute("UseSidelink", BooleanValue (True))

lteHelper.SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"))
lteHelper.SetPathlossModelAttribute ("BSAntennaHeight", DoubleValue(1.5));
lteHelper.SetPathlossModelAttribute ("SSAntennaHeight", DoubleValue(1.5));
lteHelper.Initialize ();

ulFreq = ns.lte.LteSpectrumValueHelper.GetCarrierFrequency (ulEarfcn)
print(ulFreq)

uplinkPathlossModel = lteHelper.GetUplinkPathlossModel()
print(uplinkPathlossModel)
#lossModel = uplinkPathlossModel.GetObject(lteHelper.PropagationLossModel.getTypeId())

uplinkPathlossModel.SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq))


mobility = ns.mobility.MobilityHelper()
mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", ns.core.DoubleValue(20.0),
                                  "MinY", ns.core.DoubleValue(20.0),
                                  "DeltaX", ns.core.DoubleValue(20.0),
                                  "DeltaY", ns.core.DoubleValue(20.0),
                                  "GridWidth", ns.core.UintegerValue(5),
                                  "LayoutType", ns.core.StringValue("RowFirst"))
mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel",
                               "Bounds", ns.mobility.RectangleValue(ns.mobility.Rectangle(-500, 500, -500, 500)),
                               "Speed", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                               "Pause", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"))

mobility.Install(ueNodes)

ueDevs = lteHelper.InstallUeDevice (ueNodes)

ueSidelinkConfiguration = ns.lte.LteSlUeRrc()
ueSidelinkConfiguration.SetSlEnabled (True)

preconfiguration = ns.lte.LteRrcSap.SlPreconfiguration()

preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn
preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth
preconfiguration.preconfigComm.nbPools = 1

pfactory = ns.lte.LteSlPreconfigPoolFactory()

pfactory.SetControlPeriod ("sf40");
pfactory.SetControlBitmap (0x00000000FF)
pfactory.SetControlOffset (0)
pfactory.SetControlPrbNum (22)
pfactory.SetControlPrbStart (0)
pfactory.SetControlPrbEnd (49)

pfactory.SetDataBitmap (0xFFFFFFFFFF)
pfactory.SetDataOffset (8)
pfactory.SetDataPrbNum (25)
pfactory.SetDataPrbStart (0)
pfactory.SetDataPrbEnd (49)

preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool()

ueSidelinkConfiguration.SetSlPreconfiguration (preconfiguration)
lteHelper.InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration)

internet = ns.internet.InternetStackHelper()
internet.Install (ueNodes)

groupL2Address = 255
groupAddress4 = ns.network.Ipv4Address("225.63.63.1")
#groupAddress4 = ns.internet.Ipv4AddressHelper("225.63.63.1")
#groupAddress4.SetBase(ns.network.Ipv4Address("225.63.63.1"), ns.network.Ipv4Mask("255.255.255.0"))

tft = ns.lte.LteSlTft(ns.lte.LteSlTft.BIDIRECTIONAL,groupAddress4,groupL2Address)
proseHelper.ActivateSidelinkBearer (slBearersActivationTime, ueDevs, tft)

ndnHelper = ndn.StackHelper()
ndnHelper.SetDefaultRoutes(True)
ndnHelper.InstallAll();

ndn.StrategyChoiceHelper.InstallAll("/", "/localhost/nfd/strategy/directed-geocast/%FD%01/" +str(tmin) + "/" + str(tmax))

consumerHelper = ndn.AppHelper("ns3::ndn::ConsumerCbr")
consumerHelper.SetPrefix("/v2safety/8thStreet/0,0,0/700,0,0/100")
consumerHelper.SetAttribute("Frequency", StringValue("1"))
consumerHelper.Install(ueNodes.Get(0))

producerHelper = ndn.AppHelper("ns3::ndn::Producer")
producerHelper.SetPrefix("/v2safety/8thStreet");
producerHelper.SetAttribute("PayloadSize", StringValue("50"));
producerHelper.Install(ueNodes.Get(nodeNum-1));

Simulator.Stop(Seconds(20.0))
Simulator.Run()
