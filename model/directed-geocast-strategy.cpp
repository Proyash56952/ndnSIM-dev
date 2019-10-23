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

#include "directed-geocast-strategy.hpp"
#include "daemon/fw/algorithm.hpp"
#include "daemon/common/logger.hpp"
#include "daemon/common/global.hpp"

#include <ndn-cxx/lp/geo-tag.hpp>

#include "ns3/mobility-model.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include <tuple>
#include <sstream>
#include "math.h"
#include "ns3/vector.h"
#include <chrono>
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/double.h"
#include <ctime>
#include <cstdlib>

#include <string>

namespace nfd {
namespace fw {

NFD_REGISTER_STRATEGY(DirectedGeocastStrategy);

NFD_LOG_INIT(DirectedGeocastStrategy);

DirectedGeocastStrategy::DirectedGeocastStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("DirectedGeocastStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "DirectedGeocastStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
DirectedGeocastStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/directed-geocast/%FD%01");
  return strategyName;
}

void
DirectedGeocastStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                              const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_DEBUG("ReceivedInterest: ");
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  int nEligibleNextHops = 0;

  for (const auto& nexthop : nexthops) {
    Face& outFace = nexthop.getFace();

    if ((outFace.getId() == ingress.face.getId() && outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) ||
        wouldViolateScope(ingress.face, interest, outFace)) {
      continue;
    }

    if (outFace.getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) {
      // for non-ad hoc links, send interest as usual
      this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
      NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
    }
    //NFD_LOG_DEBUG("the link type is " << outFace.getLinkType());
    else {
      std::weak_ptr<pit::Entry> pitEntryWeakPtr = pitEntry;
      auto faceId = ingress.face.getId();

      // if transmission was already scheduled, ignore the interest

      PitInfo* pi = pitEntry->insertStrategyInfo<PitInfo>().first;
      if (pi->queue.find(faceId) != pi->queue.end()) {
        NFD_LOG_DEBUG(interest << " already scheduled pitEntry-to=" << outFace.getId());
        continue;
      }

      //std::string interestName = interest.getName().toUri();
      //NFD_LOG_DEBUG("Interest Name is " << interestName);
      //bool limitTransmission = shouldLimitTransmission(interest);
      //NFD_LOG_DEBUG("The result of limit tranmission is " << limitTransmission);
      if(shouldLimitTransmission(interest)){
        NFD_LOG_DEBUG("limiting the transmission of " << interest);
	continue;
      }


      // calculate time to delay interest
      auto delay = calculateDelay(interest);
      NFD_LOG_DEBUG("Delaying by " << delay);
      if (delay > 0_s) {
        scheduler::ScopedEventId event = getScheduler().schedule(delay, [this, pitEntryWeakPtr,
                                                                         faceId, interest] {
            auto pitEntry = pitEntryWeakPtr.lock();
            auto outFace = getFaceTable().get(faceId);
            if (pitEntry == nullptr || outFace == nullptr) {
              // something bad happened to the PIT entry, nothing to process
              return;
            }

            this->sendInterest(pitEntry, FaceEndpoint(*outFace, 0), interest);
            NFD_LOG_DEBUG("delayed " << interest << " pitEntry-to=" << faceId);
          });

        // save `event` into pitEntry
        pi->queue.emplace(faceId, std::move(event));
      }
      else {
        this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
        NFD_LOG_DEBUG("Could not determine to delay interest");
        NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
      }
    }

    ++nEligibleNextHops;
  }

  if (nEligibleNextHops == 0) {
    NFD_LOG_DEBUG(interest << " from=" << ingress << " noNextHop");

    // don't support NACKs (for now or ever)

    // lp::NackHeader nackHeader;
    // nackHeader.setReason(lp::NackReason::NO_ROUTE);
    // this->sendNack(pitEntry, ingress, nackHeader);

    this->rejectPendingInterest(pitEntry);
  }
}

void
DirectedGeocastStrategy::afterReceiveLoopedInterest(const FaceEndpoint& ingress, const Interest& interest,
                                                    pit::Entry& pitEntry)
{
  // determine if interest needs to be cancelled or not

  PitInfo* pi = pitEntry.getStrategyInfo<PitInfo>();
  if (pi == nullptr) {
    NFD_LOG_DEBUG("Got looped interest, PitInfo is missing");
    return;
  }

  auto item = pi->queue.find(ingress.face.getId());
  if (item == pi->queue.end()) {
    NFD_LOG_DEBUG("Got looped interest, but no even was scheduled for the face");
    return;
  }

  if (shouldCancelTransmission(pitEntry, interest)) {
    item->second.cancel();

    // don't do anything to the PIT entry (let it expire as usual)
    NFD_LOG_DEBUG("Canceling transmission of " << interest << " via=" << ingress.face.getId());
  }
}

ndn::optional<ns3::Vector>
DirectedGeocastStrategy::getSelfPosition()
{
  auto node = ns3::NodeList::GetNode(ns3::Simulator::GetContext());
  if (node == nullptr) {
    return nullopt;
  }

  auto mobility = node->GetObject<ns3::MobilityModel>();
  if (mobility == nullptr) {
    return nullopt;
  }
    NFD_LOG_DEBUG("self position is: " << mobility->GetPosition());
  return mobility->GetPosition();
}

ndn::optional<ns3::Vector>
DirectedGeocastStrategy::extractPositionFromTag(const Interest& interest)
{
  auto tag = interest.getTag<ndn::lp::GeoTag>();
  NFD_LOG_DEBUG("the tag is " << tag);
  if (tag == nullptr) {
    return nullopt;
  }

  auto pos = tag->getPos();
   NFD_LOG_DEBUG("the psotion is " << ns3::Vector(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos)));
  return ns3::Vector(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos));
}

time::nanoseconds
DirectedGeocastStrategy::calculateDelay(const Interest& interest)
{
  auto self = getSelfPosition();
  auto from = extractPositionFromTag(interest);
  NFD_LOG_DEBUG("the interest is " << interest);
  
  if (!self || !from) {
    NFD_LOG_DEBUG("self or from position is missing");
    return 0_s;
  }
 

  // TODO
  //double distance = abs(self->GetLength() - from->GetLength());
  double distance = CalculateDistance(*self,*from);
  NFD_LOG_DEBUG("the distance is " << distance);
  double minTime = 0.01;
  double maxDist = 600;
  double maxTime = 0.1;
  if (distance < maxDist) {
    //auto waitTime = time::duration_cast<time::nanoseconds>(time::duration<double>{(minTime * (maxDist-distance)/maxDist)});
    //double randomNumber = static_cast <double> (rand()) / (static_cast <double> (RAND_MAX));
    ns3::RngSeedManager::SetSeed (3);
    ns3::SeedManager::SetRun (7);
    double min = minTime;
    double max = maxTime;

    ns3::Ptr<ns3::UniformRandomVariable> x = ns3::CreateObject<ns3::UniformRandomVariable> ();
    x->SetAttribute ("Min", ns3::DoubleValue(min));
    x->SetAttribute ("Max", ns3::DoubleValue(max));
    auto RandomNo = x->GetValue ();
    //float randomNumber = 0.003;
    NFD_LOG_DEBUG("the random number is " << RandomNo);

    auto waitTime = time::duration_cast<time::nanoseconds>(time::duration < double >{((maxTime * (maxDist - distance) / maxDist) +
                                                                        	     minTime + RandomNo)});
    // auto waitTime = ((maxTime * (maxDist-distance)/maxDist) + minTime);
    NFD_LOG_DEBUG("distance to last hop is " << distance << " meter");
    //NFD_LOG_DEBUG("distance to last hop is "<<distance<<" meter");
    NFD_LOG_DEBUG("self is at: " << self->GetLength() << " meter");
    NFD_LOG_DEBUG("from is at: " << from->GetLength() << " meter");
    NFD_LOG_DEBUG("self and from are within max limit hence delay is: " << waitTime);
    //return waitTime;
    return time::duration_cast<time::nanoseconds>(time::duration < double >{RandomNo});
  } 
  else {
    NFD_LOG_DEBUG("Minimum Delay added is: 10ms ");
    return 10_ms;
  }
}

bool
DirectedGeocastStrategy::shouldCancelTransmission(const pit::Entry& oldPitEntry, const Interest& newInterest)
{
  NFD_LOG_DEBUG("Entered into Should cancel tranmission ");
  auto self = getSelfPosition();
  auto oldFrom = extractPositionFromTag(oldPitEntry.getInterest());
  auto newFrom = extractPositionFromTag(newInterest);

  if (!self || !oldFrom || !newFrom) {
    NFD_LOG_DEBUG("self, oldFrom, or newFrom position is missing");
    return false;
  }

  // TODO
  NFD_LOG_DEBUG("self, oldform and newform are " << self->GetLength() << " " << oldFrom->GetLength() << " " << newFrom->GetLength());
  //distance calculation
  //double distanceToLasthop = (self->GetLength() - newFrom->GetLength());
  double distanceToLasthop = CalculateDistance(*self,*newFrom);
  NFD_LOG_DEBUG("distance to last hop is " << distanceToLasthop);
  double distanceToOldhop = CalculateDistance(*self,*oldFrom);
  //double distanceToOldhop = (self->GetLength() - oldFrom->GetLength());
  NFD_LOG_DEBUG("distance to old hop is " << distanceToOldhop);
  double distanceBetweenLasthops = CalculateDistance(*newFrom,*oldFrom);
  //double distanceBetweenLasthops = (newFrom->GetLength() - oldFrom->GetLength());
  NFD_LOG_DEBUG("distance between last hops is " << distanceBetweenLasthops);

  //Angle calculation
  double angleRad = acos((pow(distanceToOldhop, 2) + pow(distanceBetweenLasthops, 2) - pow(distanceToLasthop, 2)) /
                    (2 * distanceToOldhop * distanceBetweenLasthops));
  double angleDeg = angleRad * 180 / 3.141592;
  // double Angle_Deg = 91.00;
  NFD_LOG_DEBUG("angle is " << angleDeg);

  // Projection Calculation
  double cosineAngleAtSelf = (pow(distanceToOldhop, 2) - pow(distanceToLasthop, 2) + pow(distanceBetweenLasthops, 2)) /
                    (2 * distanceToOldhop * distanceBetweenLasthops);
  NFD_LOG_DEBUG("cosine-angle is " << cosineAngleAtSelf);
  double projection = abs(distanceBetweenLasthops * cosineAngleAtSelf);
  NFD_LOG_DEBUG("projection is " << projection);
            
  if (angleDeg >= 90) {
    NFD_LOG_DEBUG("Interest need not to be cancelled");
    return false;
   }
  else if (projection > distanceToOldhop) {
    NFD_LOG_DEBUG("Interest need to be cancelled");
    return true;
   }
  NFD_LOG_DEBUG("Interest need not to be cancelled");
  return false;
}

ndn::optional<ns3::Vector>
DirectedGeocastStrategy::parsingCoordinate(std::string s)
{
  std::string delim = "%2C";
  double content[3];
  int i =0;
  auto start = 0U;
  auto end = s.find(delim);
  while (end != std::string::npos){
    std::string temp = s.substr(start, end-start);    
//std::cout << s.substr(start, end - start) << std::endl;
    start = end + delim.length();
    end = s.find(delim, start);
    content[i] = atof(temp.c_str());
    i++;
  }
  content[i] = atof(s.substr(start,end).c_str());
 
  ndn::optional<ns3::Vector> result = ns3::Vector3D(content[0],content[1],content[2]);
  return result;
  
}
bool
DirectedGeocastStrategy::shouldLimitTransmission(const Interest& interest)
{
  auto name = interest.getName();
  //auto dst = std::string(name.get(-2).value(), name.get(-2).value_size());
  std::string limitStr = name.get(-2).toUri();
  std::string dest = name.get(-3).toUri();
  std::string src = name.get(-4).toUri();
  double limit = atof(limitStr.c_str());
  ndn::optional<ns3::Vector> destination = parsingCoordinate(dest);
  ndn::optional<ns3::Vector> source = parsingCoordinate(src);
  auto self = getSelfPosition();
  double distSrcDest = CalculateDistance(*source,*destination);
  double distCurSrc = CalculateDistance(*source,*self);
  double distCurDest = CalculateDistance(*destination, *self);
  double cosineAngle = (pow(distCurSrc, 2) - pow(distCurDest, 2) + pow(distSrcDest, 2)) /
                    (2 * distCurSrc * distSrcDest);
  double angle = (acos(cosineAngle)*180)/3.141592;
  double projection = distCurSrc * cosineAngle;
  NFD_LOG_DEBUG("the angle and distances are: "<< angle <<" " << distSrcDest << " " <<distCurSrc << " " <<distCurDest << " " <<projection);
  if(angle > 90){
    return true;
  }
  else if(projection > distSrcDest+limit){
    return true;
  }


  //if(distCurSrc > distSrcDest+limit){
    //return true;	      
  //}
  //double distCurDest = CalculateDistance(*destination, *self);

 return false;
}

} // namespace fw
} // namespace nfd

