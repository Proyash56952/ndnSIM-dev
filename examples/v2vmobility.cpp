#include "ns3/lte-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"
#include <cfloat>
#include <sstream>
#include <cmath>
#include "ns3/ndnSIM-module.h"
#include "ns3/random-direction-2d-mobility-model.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("LteSlOutOfCovrg");

int main (int argc, char *argv[])
{
  Time simTime = Seconds (6);
  bool enableNsLogs = false;
  bool useIPv6 = false;
  //double distance=atoi(argv[1]);

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
 // cmd.AddValue("distance", "Distance apart to place nodes (in meters).",distance);

  cmd.Parse (argc, argv);

  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16));
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); //The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1));
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); //use default Trp index of 0

  //Set the frequency
  uint32_t ulEarfcn = 2000;
  uint16_t ulBandwidth = 50;

  // Set error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  // parse again so we can override input file default values via command line
  cmd.Parse (argc, argv);

  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);

      LogComponentEnable ("LteUeRrc", logLevel);
      LogComponentEnable ("LteUeMac", logLevel);
      LogComponentEnable ("LteSpectrumPhy", logLevel);
      LogComponentEnable ("LteUePhy", logLevel);
    }

  //Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (0.0));

  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (2.0);

  //Create the helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  //Create and set the EPC helper
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ////Create Sidelink helper and set lteHelper
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  //Enable Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

  //Set pathloss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));
  // channel model initialization
  lteHelper->Initialize ();

  // Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (ulEarfcn);
  NS_LOG_LOGIC ("UL freq: " << ulFreq);
  Ptr<Object> uplinkPathlossModel = lteHelper->GetUplinkPathlossModel ();
  Ptr<PropagationLossModel> lossModel = uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ABORT_MSG_IF (lossModel == NULL, "No PathLossModel");
  bool ulFreqOk = uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
  if (!ulFreqOk)
    {
      NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
    }

  NS_LOG_INFO ("Deploying UE's...");

  //Create nodes (UEs)
  NodeContainer ueNodes;
  ueNodes.Create (6);
  NS_LOG_INFO ("UE 1 node id = [" << ueNodes.Get (0)->GetId () << "]");
  NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");
  NS_LOG_INFO ("UE 3 node id = [" << ueNodes.Get (2)->GetId () << "]");

  //Position of the nodes
  Ptr<ListPositionAllocator> positionAllocUe1 = CreateObject<ListPositionAllocator> ();
  positionAllocUe1->Add (Vector (-800.0, -50.0, 0.0));
  Ptr<ListPositionAllocator> positionAllocUe2 = CreateObject<ListPositionAllocator> ();
  positionAllocUe2->Add (Vector (-700.0, -50.0, 0.0));
  Ptr<ListPositionAllocator> positionAllocUe3 = CreateObject<ListPositionAllocator> ();

  positionAllocUe3->Add (Vector (-500.0, 50.0, 0.0));
  

  positionAllocUe3->Add (Vector (-800.0, -200.0, 0.0));
  Ptr<ListPositionAllocator> positionAllocUe5 = CreateObject<ListPositionAllocator> ();
  positionAllocUe5->Add (Vector (-600.0, -200.0, 0.0));
  Ptr<ListPositionAllocator> positionAllocUe4 = CreateObject<ListPositionAllocator> ();
  positionAllocUe4->Add (Vector (0.0, 25.0, 0.0));

  //Install mobility


  MobilityHelper mobilityUe1;
  /*
  mobilityUe1.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-400, 0, 0, 0)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=0.0001]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  */
  //mobilityUe1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobilityUe1.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue (-800.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue (-50.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue (-750.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue (-49.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue (0.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue (16.67));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue (5));
  
  mobilityUe1.SetPositionAllocator (positionAllocUe1);
  mobilityUe1.Install (ueNodes.Get (0));

  MobilityHelper mobilityUe2;
  //mobilityUe2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  
  mobilityUe2.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue (-500.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue (-50.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue (-450.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue (-49.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue (0.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue (15));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue (3));
  mobilityUe2.SetPositionAllocator (positionAllocUe2);
  mobilityUe2.Install (ueNodes.Get (1));

  MobilityHelper mobilityUe3;
  mobilityUe3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  
  mobilityUe3.SetPositionAllocator (positionAllocUe3);
  mobilityUe3.Install (ueNodes.Get (2));
  mobilityUe3.Install (ueNodes.Get (3));



  MobilityHelper mobilityUe5;
  mobilityUe5.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  
  mobilityUe5.SetPositionAllocator (positionAllocUe5);
  mobilityUe5.Install (ueNodes.Get (4));

  MobilityHelper mobilityUe4;
  //mobilityUe4.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobilityUe4.SetMobilityModel ("ns3::SteadyStateRandomWaypointMobilityModel");
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinX", DoubleValue (-25.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinY", DoubleValue (24.5));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxX", DoubleValue (25.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxY", DoubleValue (25.5));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue (0.0));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed", DoubleValue (5));
  Config::SetDefault ("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed", DoubleValue (0.5));
  mobilityUe4.SetPositionAllocator (positionAllocUe4);
  mobilityUe4.Install (ueNodes.Get (5));

  /*
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> initialAlloc = CreateObject<ListPositionAllocator> ();
  initialAlloc->Add(Vector(0.0,0.0,0.0));
  initialAlloc->Add(Vector(400.0,0.0,0.0));
  initialAlloc->Add(Vector(300.0,100.0,0.0));
  initialAlloc->Add(Vector(800.0,0.0,0.0));
  mobility.SetPositionAllocator(initialAlloc);
  //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)) ,
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  //mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel",
                            //"Bounds", Rectangle(0, 1000, 0, 1000),
                            //"Speed", "2000",
                            //"Pause", "0.2");
  //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //"Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install(ueNodes.Get (0));
  mobility.Install(ueNodes.Get (1));
  mobility.Install(ueNodes.Get (2));
  mobility.Install(ueNodes.Get (3));
   */





  //Install LTE UE devices to the nodes
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);

  //Sidelink pre-configuration for the UEs
  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  LteRrcSap::SlPreconfiguration preconfiguration;

  preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
  preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
  preconfiguration.preconfigComm.nbPools = 1;

  LteSlPreconfigPoolFactory pfactory;

  //Control
  pfactory.SetControlPeriod ("sf40");
  pfactory.SetControlBitmap (0x00000000FF); //8 subframes for PSCCH
  pfactory.SetControlOffset (0);
  pfactory.SetControlPrbNum (22);
  pfactory.SetControlPrbStart (0);
  pfactory.SetControlPrbEnd (49);

  //Data
  pfactory.SetDataBitmap (0xFFFFFFFFFF);
  pfactory.SetDataOffset (8); //After 8 subframes of PSCCH
  pfactory.SetDataPrbNum (25);
  pfactory.SetDataPrbStart (0);
  pfactory.SetDataPrbEnd (49);

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

  InternetStackHelper internet;
  internet.Install (ueNodes);
  uint32_t groupL2Address = 255;
  Ipv4Address groupAddress4 ("225.63.63.1");     //use multicast address as destination

  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  // set the default gateway for the UE
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u) {
    Ptr<Node> ueNode = ueNodes.Get(u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress(), 1);
  }

  Address remoteAddress = InetSocketAddress (groupAddress4, 8000);
  Address localAddress = InetSocketAddress (Ipv4Address::GetAny (), 8000);
  Ptr<LteSlTft> tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress4, groupL2Address);

  ///*** Configure applications ***///

  //Set Sidelink bearers
  proseHelper->ActivateSidelinkBearer (slBearersActivationTime, ueDevs, tft);

  ///*** End of application configuration ***///
  ::ns3::ndn::StackHelper helper;
  helper.SetDefaultRoutes(true);
  helper.InstallAll();

  //* Choosing forwarding strategy *//
  ns3::ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/directed-geocast");

 //Will add cost231Propagationloss model loss here for and packet loss

  // Consumer
  ::ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  //consumerHelper.SetPrefix("/v2safety/8thStreet/parking");
  consumerHelper.SetPrefix("/v2vsafety/8st_107ave/east_crosswalk/pedestrian/1");
  consumerHelper.SetAttribute("Frequency", StringValue("0.1")); // 10 interests a second
  consumerHelper.Install(ueNodes.Get(0));                        // first node

  // Producer
  ::ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/v2vsafety/8st_107ave/east_crosswalk/pedestrian/1");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(ueNodes.Get(5));


  Simulator::Stop (Seconds(20));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;

}

