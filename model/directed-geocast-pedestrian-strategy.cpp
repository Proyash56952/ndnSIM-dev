/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "directed-geocast-pedestrian-strategy.hpp"
#include "daemon/fw/algorithm.hpp"
#include "daemon/common/logger.hpp"
#include "daemon/common/global.hpp"

#include <ndn-cxx/lp/geo-tag.hpp>

#include <ndn-cxx/interest.hpp>
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include <tuple>
#include <sstream>
#include "math.h"
#include "ns3/vector.h"
#include <chrono>
#include "ns3/double.h"
#include <ctime>
#include <cstdlib>

#include "../scenarios/v2v-position-getter.hpp"

#include <string>

namespace nfd {
namespace fw {

//ndn::util::Signal<DirectedGeocastStrategy, Name, int, double, double> DirectedGeocastStrategy::onAction;

NFD_REGISTER_STRATEGY(DirectedGeocastPedestrianStrategy);

NFD_LOG_INIT(DirectedGeocastPedestrianStrategy);

DirectedGeocastPedestrianStrategy::DirectedGeocastPedestrianStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    // NDN_THROW(std::invalid_argument("DirectedGeocastStrategy does not accept parameters"));
    m_minTime = boost::lexical_cast<double>(parsed.parameters[0]);
    m_maxTime = boost::lexical_cast<double>(parsed.parameters[1]);
  }

  // if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
  //   NDN_THROW(std::invalid_argument(
  //     "DirectedGeocastStrategy does not support version " + to_string(*parsed.version)));
  // }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));

  m_randVar = ns3::CreateObject<ns3::UniformRandomVariable>();
  m_randVar->SetAttribute ("Min", ns3::DoubleValue(m_minTime));
  m_randVar->SetAttribute ("Max", ns3::DoubleValue(m_maxTime));
}

const Name&
DirectedGeocastPedestrianStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/directed-geocast-pedestrian/%FD%01");
  return strategyName;
}

void
DirectedGeocastPedestrianStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                              const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("ReceivedInterest: ");
}

void
DirectedGeocastPedestrianStrategy::afterReceiveLoopedInterest(const FaceEndpoint& ingress, const Interest& interest,
                                                    pit::Entry& pitEntry)
{
  NDN_LOG_DEBUG("Received Looped Inteest: ");
}

} // namespace fw
} // namespace nfd

