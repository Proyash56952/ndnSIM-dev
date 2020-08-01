#include "custom-helper.hpp"

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include "ns3/log.h"
#include "ns3/unused.h"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/constant-velocity-mobility-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomHelper");

CustomHelper::CustomHelper()
{
  //Configure the UE for UE_SELECTED scenario
  Config::SetDefault("ns3::LteUeMac::SlGrantMcs", UintegerValue(16));
  Config::SetDefault("ns3::LteUeMac::SlGrantSize", UintegerValue(5));
  Config::SetDefault("ns3::LteUeMac::Ktrp", UintegerValue(1));
  Config::SetDefault("ns3::LteUeMac::UseSetTrp", BooleanValue(true));

  // Set the frequency
  uint32_t ulEarfcn = 18100;
  uint16_t ulBandwidth = 75;

  // Set error models
  Config::SetDefault("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue(true));
  Config::SetDefault("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue(true));
  Config::SetDefault("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue(false));

  // Set the UEs power in dBm
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue(23.0));

  // Create the helpers
  m_lteHelper = CreateObject<LteHelper>();

  // Create and set the EPC helper
  m_epcHelper = CreateObject<PointToPointEpcHelper> ();
  m_lteHelper->SetEpcHelper(m_epcHelper);

  ////Create Sidelink helper and set lteHelper
  m_proseHelper = CreateObject<LteSidelinkHelper> ();
  m_proseHelper->SetLteHelper(m_lteHelper);

  // Enable Sidelink
  m_lteHelper->SetAttribute ("UseSidelink", BooleanValue(true));

  // Set pathloss model
  m_lteHelper->SetAttribute("PathlossModel", StringValue("ns3::Cost231PropagationLossModel"));

  // channel model initialization
  m_lteHelper->Initialize();


  // Since we are not installing eNB, we need to set the frequency attribute of pathloss model here
  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency(ulEarfcn);

  m_uplinkPathlossModel = m_lteHelper->GetUplinkPathlossModel ();
  Ptr<PropagationLossModel> lossModel = m_uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ABORT_MSG_IF(lossModel == NULL, "No PathLossModel");
  m_uplinkPathlossModel->SetAttributeFailSafe("Frequency", DoubleValue(ulFreq));

  // Sidelink pre-configuration for the UEs
  m_ueSidelinkConfiguration = CreateObject<LteSlUeRrc> ();
  m_ueSidelinkConfiguration->SetSlEnabled (true);

  LteRrcSap::SlPreconfiguration preconfiguration;

  preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
  preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
  preconfiguration.preconfigComm.nbPools = 1;

  LteSlPreconfigPoolFactory pfactory;

  //Control
  pfactory.SetControlPeriod  ("sf40");
  pfactory.SetControlBitmap  (0x00000000FF); //8 subframes for PSCCH
  pfactory.SetControlOffset  (0);
  pfactory.SetControlPrbNum  (22);
  pfactory.SetControlPrbStart(0);
  pfactory.SetControlPrbEnd  (49);

  //Data
  pfactory.SetDataBitmap  (0xFFFFFFFFFF);
  pfactory.SetDataOffset  (8); //After 8 subframes of PSCCH
  pfactory.SetDataPrbNum  (25);
  pfactory.SetDataPrbStart(0);
  pfactory.SetDataPrbEnd  (49);

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();

  m_ueSidelinkConfiguration->SetSlPreconfiguration(preconfiguration);

  uint32_t groupL2Address = 255;
  Ipv4Address groupAddress4("225.63.63.1");     //use multicast address as destination
  m_tft = Create<LteSlTft>(LteSlTft::BIDIRECTIONAL, groupAddress4, groupL2Address);
}

void
CustomHelper::Install(Ptr<Node> node) const
{
  // Install LTE UE devices to the nodes
  NetDeviceContainer ueDevs = m_lteHelper->InstallUeDevice(node);

  m_lteHelper->InstallSidelinkConfiguration(ueDevs, m_ueSidelinkConfiguration);

  m_internetHelper.Install(node);

  auto ueIpIface = m_epcHelper->AssignUeIpv4Address(ueDevs);

  // set the default gateway for the UE
  auto ueStaticRouting = m_ipv4RoutingHelper.GetStaticRouting(node->GetObject<Ipv4>());
  ueStaticRouting->SetDefaultRoute(m_epcHelper->GetUeDefaultGatewayAddress(), 1);

  // Sidelink bearers activation time
  Time slBearersActivationTime = Seconds(2.0);

  m_proseHelper->ActivateSidelinkBearer(slBearersActivationTime, ueDevs, m_tft);
}

} // namespace ns3
