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

ndn::util::Signal<DirectedGeocastStrategy, Name, int, double, double> DirectedGeocastStrategy::onAction;

NFD_REGISTER_STRATEGY(DirectedGeocastStrategy);

NFD_LOG_INIT(DirectedGeocastStrategy);

DirectedGeocastStrategy::DirectedGeocastStrategy(Forwarder& forwarder, const Name& name)
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
DirectedGeocastStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/directed-geocast/%FD%01");
  return strategyName;
}

void
DirectedGeocastStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                                              const shared_ptr<pit::Entry>& pitEntry)
{
  double posX =0.0;
  double posY =0.0;
  ndn::optional<ns3::Vector> pos = getSelfPosition();
  if(pos) {
    posX = pos->x;
    posY = pos->y;
  }

  this->onAction(interest.getName(), Received, posX, posY);

  NFD_LOG_DEBUG("ReceivedInterest: ");
  //std::cout<< "Received Interest" <<std::endl;

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
      this->onAction(interest.getName(), Sent, posX, posY);

      NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
    }

    else {
      std::weak_ptr<pit::Entry> pitEntryWeakPtr = pitEntry;
      auto faceId = ingress.face.getId();

      // if transmission was already scheduled, ignore the interest

      PitInfo* pi = pitEntry->insertStrategyInfo<PitInfo>().first;
      if (pi->queue.find(faceId) != pi->queue.end()) {
        NFD_LOG_DEBUG(interest << " already scheduled pitEntry-to=" << outFace.getId());
        //std::cerr << "Impossible point" << std::endl;
        auto item = pi->queue.find(ingress.face.getId());
        if (item == pi->queue.end()) {
          NFD_LOG_DEBUG("Got the modified looped interest, but no even was scheduled for the face");
          return;
        }

        if (shouldCancelTransmission(*pitEntry, interest)) {
          item->second.cancel();
          //std::cout<<"cancelling:"<<std::endl;
          //this->onAction(interest.getName(), Canceled, posX1, posY1);

          // don't do anything to the PIT entry (let it expire as usual)
          NFD_LOG_DEBUG("Canceling transmission of " << interest << " via=" << ingress.face.getId());
          pi->queue.erase(item);
        }
        //shouldCancelTransmission(*pitEntry ,interest);
        continue;
      }

      if(shouldNotTransmit(*pitEntry, interest)) {
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
        double posX =0.0;
        double posY =0.0;
        ndn::optional<ns3::Vector> pos = getSelfPosition();
        if(pos){
          posX = pos->x;
          posY = pos->y;
        }
        //std::cout << x;
        this->onAction(interest.getName(), Sent, posX, posY);
        NFD_LOG_DEBUG("delayed " << interest << " pitEntry-to=" << faceId);
        });

        // save `event` into pitEntry
        pi->queue.emplace(faceId, std::move(event));
      }
      else {
        this->sendInterest(pitEntry, FaceEndpoint(outFace, 0), interest);
        double posX =0.0;
        double posY =0.0;
        ndn::optional<ns3::Vector> pos = getSelfPosition();
        if(pos){
          posX = pos->x;
          posY = pos->y;
        }
        //std::cout << x;
        this->onAction(interest.getName(), Sent, posX, posY);


        //this->onAction(interest.getName(), Sent, posx, posy);
        NFD_LOG_DEBUG("Could not determine to delay interest");
        NFD_LOG_DEBUG(interest << " from=" << ingress << " pitEntry-to=" << outFace.getId());
      }
    }

    ++nEligibleNextHops;
  }
}

void
DirectedGeocastStrategy::afterReceiveLoopedInterest(const FaceEndpoint& ingress, const Interest& interest,
                                                    pit::Entry& pitEntry)
{
  double posX1 = 0.0;
  double posY1 = 0.0;
  ndn::optional<ns3::Vector> pos = getSelfPosition();
  if(pos) {
    posX1 = pos->x;
    posY1 = pos->y;
  }
  this->onAction(interest.getName(), ReceivedDup, posX1, posY1);
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

  if (modifiedshouldCancelTransmission(pitEntry, interest)) {
    item->second.cancel();
    this->onAction(interest.getName(), Canceled, posX1, posY1);

    // don't do anything to the PIT entry (let it expire as usual)
    NFD_LOG_DEBUG("Canceling transmission of " << interest << " via=" << ingress.face.getId());
    pi->queue.erase(item);
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
  //NFD_LOG_DEBUG("the tag is " << tag);
  if (tag == nullptr) {
    return nullopt;
  }

  auto pos = tag->getPos();
  NFD_LOG_DEBUG("the position is " << ns3::Vector(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos)));
  return ns3::Vector(std::get<0>(pos), std::get<1>(pos), std::get<2>(pos));
}

time::nanoseconds
DirectedGeocastStrategy::calculateDelay(const Interest& interest)
{
  auto self = getSelfPosition();
  auto from = extractPositionFromTag(interest);
  //NFD_LOG_DEBUG("the interest is " << interest);

  if (!self || !from) {
    NFD_LOG_DEBUG("self or from position is missing");
    return 0_s;
  }

  double maxDist = 200;
  double distance = CalculateDistance(*self,*from);
    //std::cout<<distance<<std::endl;
  if (distance < maxDist) {
    auto RandomNo = m_randVar->GetValue ();
    return time::duration_cast<time::nanoseconds>(time::duration<double>{RandomNo});
  }

  else {
    NFD_LOG_DEBUG("Minimum Delay added is: 10ms ");
    return 10_ms;
  }
}

time::nanoseconds
DirectedGeocastStrategy::prevcalculateDelay(const Interest& interest)
{
  auto self = getSelfPosition();
  auto from = extractPositionFromTag(interest);
  //NFD_LOG_DEBUG("the interest is " << interest);

  if (!self || !from) {
    NFD_LOG_DEBUG("self or from position is missing");
    return 0_s;
  }

  double maxDist = 200;
  double distance = CalculateDistance(*self,*from);
    //std::cout<<distance<<std::endl;
  if (distance < maxDist) {
    auto RandomNo = m_randVar->GetValue ();
    auto delay = (m_maxTime) * ((maxDist - distance) / maxDist) + RandomNo;
    std::cout<< time::duration_cast<time::nanoseconds>(time::duration<double>{delay}) <<std::endl;
    return time::duration_cast<time::nanoseconds>(time::duration<double>{delay});
  }

  else {
    NFD_LOG_DEBUG("Minimum Delay added is: 10ms ");
    return 10_ms;
  }
}

bool
DirectedGeocastStrategy::shouldCancelTransmission(const pit::Entry& oldPitEntry, const Interest& newInterest)
{
  auto self = getSelfPosition();
  auto oldFrom = extractPositionFromTag(oldPitEntry.getInterest());
  auto newFrom = extractPositionFromTag(newInterest);

  if (!self || !oldFrom || !newFrom) {
    NFD_LOG_DEBUG("self, oldFrom, or newFrom position is missing");
    return false;
  }

  //distance calculation
  double distanceToLasthop = CalculateDistance(*self, *newFrom);
  double distanceToOldhop = CalculateDistance(*self, *oldFrom);
  double distanceBetweenLasthops = CalculateDistance(*newFrom, *oldFrom);

  //Angle calculation
  // double angleRad = acos((pow(distanceToOldhop, 2) + pow(distanceBetweenLasthops, 2) - pow(distanceToLasthop, 2)) /
  //                   (2 * distanceToOldhop * distanceBetweenLasthops));
  // double angleDeg = angleRad * 180 / 3.141592;

  // Projection Calculation
  double cosineAngleAtSelf = (pow(distanceToOldhop, 2) - pow(distanceToLasthop, 2) + pow(distanceBetweenLasthops, 2)) /
                    (2 * distanceToOldhop * distanceBetweenLasthops);
  NFD_LOG_DEBUG("cosine-angle is " << cosineAngleAtSelf);
  double projection = abs(distanceBetweenLasthops * cosineAngleAtSelf);
  NFD_LOG_DEBUG("projection is " << projection);

  /*(if (angleDeg >= 90) {
    NFD_LOG_DEBUG("Interest need not to be cancelled due to angle");
    return false;
   }*/
  if (projection > distanceToOldhop) {
    NFD_LOG_DEBUG("Interest need to be cancelled due to distance");
    return true;
   }
  NFD_LOG_DEBUG("Interest need not to be cancelled");
  return false;
}

bool
DirectedGeocastStrategy::modifiedshouldCancelTransmission(const pit::Entry& oldPitEntry, const Interest& newInterest)
{
  auto cur = getSelfPosition();
  auto prev = extractPositionFromTag(newInterest);

  ns3::Vector destination;
  ::ndn::Vector t(newInterest.getName()[-3]);
  destination = ns3::Vector(t.x, t.y, t.z);

  if (!cur || !prev) {
    NFD_LOG_DEBUG("self, oldFrom, or newFrom position is missing");
    return false;
  }

  //distance calculation
  double distanceCurDest = CalculateDistance(*cur, destination);
  double distancePrevDest = CalculateDistance(*prev, destination);
  double distanceCurPrev = CalculateDistance(*cur, *prev);

  // this check means if the previous node from which the current node receives the Interest, is nearer to the destination,
  // then, it should cancel the scheduled rebroadcast as a node closer to the destination already rebroadcast that.
  if (distanceCurDest > distancePrevDest) {
    NFD_LOG_DEBUG("Interest need to be cancelled due to distance");
    return true;
   }

  // this check means, if a node receive a looped interest from another node that is in a very close proximity, then it might
  // cancel its scehduled rebroadcast.
  if (distanceCurPrev < 20) {
    NFD_LOG_DEBUG ("Internet need to be cancelled due to close proximity");
    return true;
  }
  NFD_LOG_DEBUG("Interest need not to be cancelled");
  return false;
}

bool
DirectedGeocastStrategy::shouldLimitTransmission(const Interest& interest)
{
  NFD_LOG_DEBUG("Entered in should Limit Tranmission");
  auto newFrom = extractPositionFromTag(interest);
  if (!newFrom) {
    // originator
    return false;
  }

  // Interest name format
  // /v2vSafety/[targetVector]/[sourceVector]/[targetTimePointIsoString]/[LimitNumber]

  ns3::Vector destination;
  ns3::Vector source;
  double limit = 0;

  try {
    if (interest.getName().size() < 5) {
      NFD_LOG_DEBUG("Interest doesn't look like v2vsafety (not enough components)");
      return false;
    }
    limit = interest.getName()[-1].toNumber();
    // strategy ignores time
    ::ndn::Vector s(interest.getName()[-3]);
    ::ndn::Vector t(interest.getName()[-4]);

    source = ns3::Vector(s.x, s.y, s.z);
    destination = ns3::Vector(t.x, t.y, t.z);
  }
  catch (const tlv::Error&) {
    NFD_LOG_DEBUG("Interest doesn't look like v2vsafety (could not correctly parse name components)");
    return false;
  }

  auto self = getSelfPosition();
  if (!self) {
    NFD_LOG_DEBUG("self position is missing");
    return false;
  }

  double distSrcDest = CalculateDistance(source, destination);
  double distCurSrc = CalculateDistance(source, *self);
  double distCurDest = CalculateDistance(destination, *self);
  double cosineAngle = (pow(distCurSrc, 2) - pow(distCurDest, 2) + pow(distSrcDest, 2)) /
                    (2 * distCurSrc * distSrcDest);
  double angle = (acos(cosineAngle)*180)/3.141592;
  double projection = distCurSrc * cosineAngle;
    //std::cout<< distSrcDest <<" " <<distCurSrc <<" "<<distCurDest<<std::endl;
  if (distCurSrc < 5 || distCurDest < 5) {
    return true;
  }

  if (isnan(angle) || angle < 0 || angle > 90) {
    NFD_LOG_DEBUG("limiting for angle " << angle);
    return true;
  }
  else if (projection > distSrcDest + limit) {
    NFD_LOG_DEBUG("limiting for distance");
    return true;
  }

 return false;
}

bool
DirectedGeocastStrategy::shouldNotTransmit(const pit::Entry& oldPitEntry, const Interest& interest)
{
  NFD_LOG_DEBUG("Entered in should not transmit");
    //std::cout<<"Entered in should not transmit"<<std::endl;
  auto newFrom = extractPositionFromTag(interest);
  auto oldFrom = extractPositionFromTag(oldPitEntry.getInterest());
  if (!newFrom) {
    if( !oldFrom){
      std::cout<< "originator"<<std::endl;
      return false;// originator
    }
    else {
      std::cout<< "oldInterest " <<oldPitEntry.getInterest() <<std::endl;
      std::cout<< "newInterest " <<interest <<std::endl;
      if(interest.matchesInterest(oldPitEntry.getInterest()))
        return true;
      return false;
    }
  }

  // Interest name format
  // /v2vSafety/[targetVector]/[targetTimePointIsoString]/[LimitNumber]

  ns3::Vector destination;
  ns3::Vector source;
  double limit = 0;

  try {
    if (interest.getName().size() < 4) {
      NFD_LOG_DEBUG("Interest doesn't look like v2vsafety (not enough components)");
      return false;
    }
    limit = interest.getName()[-1].toNumber();
    // strategy ignores time
    //::ndn::Vector s(interest.getName()[-3]);
    ::ndn::Vector t(interest.getName()[-3]);

    //source = ns3::Vector(s.x, s.y, s.z);
    destination = ns3::Vector(t.x, t.y, t.z);
  }
  catch (const tlv::Error&) {
    NFD_LOG_DEBUG("Interest doesn't look like v2vsafety (could not correctly parse name components)");
    return false;
  }

  auto self = getSelfPosition();
  if (!self) {
    NFD_LOG_DEBUG("self position is missing");
    return false;
  }

  double distSrcDest = CalculateDistance(*newFrom, destination);
  double distCurSrc = CalculateDistance(*newFrom, *self);
  double distCurDest = CalculateDistance(destination, *self);
  double cosineAngle = (pow(distCurSrc, 2) - pow(distCurDest, 2) + pow(distSrcDest, 2)) /
                    (2 * distCurSrc * distSrcDest);
  double angle = (acos(cosineAngle)*180)/3.141592;
  double projection = distCurSrc * cosineAngle;
    //std::cout<< distSrcDest <<" " <<distCurSrc <<" "<<distCurDest<<std::endl;
  if (distCurSrc < 10 || distCurDest < 10) {
    return true;
  }
  //std::cout<<angle<<std::endl;
  if (isnan(angle) || angle < 0 || angle > 90) {
    NFD_LOG_DEBUG("limiting for angle " << angle);
    return true;
  }

  if (distSrcDest < 120) {
    //std::cout << "limiting for less than 1 hop distance" << std::endl;
    return true;
  }
  if (projection > distSrcDest + limit) {
    NFD_LOG_DEBUG("limiting for distance");
    return true;
  }

 return false;
}


void
DirectedGeocastStrategy::satisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                                         const FaceEndpoint& ingress, const Data& data,
                                         std::set<std::pair<Face*, EndpointId>>& satisfiedDownstreams,
                                         std::set<std::pair<Face*, EndpointId>>& unsatisfiedDownstreams)
{
  NFD_LOG_DEBUG("satisfyInterest pitEntry=" << pitEntry->getName()
                << " in=" << ingress << " data=" << data.getName());

  NFD_LOG_DEBUG("onIncomingData matching=" << pitEntry->getName());

  auto now = time::steady_clock::now();

  std::weak_ptr<pit::Entry> pitEntryWeakPtr = pitEntry;

  std::vector<Face*> inRecordsToDelete;
  // remember pending downstreams
  for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
    if (inRecord.getExpiry() > now) {

      if (inRecord.getFace().getLinkType() != ndn::nfd::LINK_TYPE_AD_HOC) {
        // normal processing, satisfy (request satisfying) the interest right away
        satisfiedDownstreams.emplace(&inRecord.getFace(), 0);
      }
      else {
        // only do it for ad hoc links, like LTE SideLink
        unsatisfiedDownstreams.emplace(&inRecord.getFace(), 0);

        PitInfo* pi = pitEntry->insertStrategyInfo<PitInfo>().first;

        auto it = pi->dataSendingQueue.find(inRecord.getFace().getId());
        if (it != pi->dataSendingQueue.end()) {
          // have a scheduled data to the face, but we not necessarily want to cancel it.
          // definitely should not cancel, if we receive data on another (ad hoc or not) face
          // however, even if we received on the same, we may keep it (though for now we going to cancel)

          if (ingress.face.getId() == inRecord.getFace().getId()) {
            // cancel
            pi->dataSendingQueue.erase(it); // this will remove entry from list and automatically cancel the transmission

            // and don't forget to remove the entry
            inRecordsToDelete.push_back(&inRecord.getFace());
          }
          else {
            // do nothing. Some other data already scheduled and nothing to do
          }
        }
        else {
          // need to schedule sending data, so we have a chance to cancel transmission

          auto delay = time::duration_cast<time::nanoseconds>(time::duration<double>{m_randVar->GetValue()});
          scheduler::ScopedEventId event =
            getScheduler().schedule(delay, [this, pitEntryWeakPtr,
                                            faceId=inRecord.getFace().getId(), data] {
                                      auto pitEntry = pitEntryWeakPtr.lock();
                                      auto outFace = getFaceTable().get(faceId);
                                      if (pitEntry == nullptr || outFace == nullptr) {
                                        // something bad happened to the PIT entry, nothing to process
                                        return;
                                      }

                                      this->sendData(pitEntry, data, FaceEndpoint(*outFace, 0));
                                      // if it is the last incoming entry in PIT, it will be automatically deleted by sendData
                                    });

          pi->dataSendingQueue.emplace(inRecord.getFace().getId(), std::move(event));
        }
      }
    }
  }

  for (const auto& face : inRecordsToDelete) {
    pitEntry->deleteInRecord(*face);
  }

  // invoke PIT satisfy callback
  beforeSatisfyInterest(pitEntry, ingress, data);
}

} // namespace fw
} // namespace nfd
