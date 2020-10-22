/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-consumer.hpp"

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/lp/tags.hpp>

namespace ndn {

NDN_LOG_INIT(v2v.Consumer);

V2vConsumer::V2vConsumer(const std::string& id, std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain)
  : m_id(id)
  , m_positionGetter(positionGetter)
  // , m_keyChain(keyChain)
  , m_scheduler(m_face.getIoService())
{
  std::cerr << "[" << time::system_clock::now() << "] starting consumer" << std::endl;

  // m_scheduler.schedule(time::seconds(2), [this] {
  //     m_face.expressInterest(Interest("/hello/world"),
  //                            std::bind([] { std::cout << "Hello!" << std::endl; }),
  //                            std::bind([] { std::cout << "NACK!" << std::endl; }),
  //                            std::bind([] { std::cout << "Bye!.." << std::endl; }));
  //   });
  //scheduleNext();
}

void
V2vConsumer::scheduleNext()
{
  m_scheduler.schedule(time::seconds(1), [this] {
      std::cerr << "[" << time::system_clock::now() << "] Node " << m_id << " is at " << m_positionGetter->getPosition() << std::endl;

      scheduleNext();
    });
}


V2vConsumer::~V2vConsumer()
{
  std::cerr << "[" << time::system_clock::now() << "] stopping consumer" << std::endl;
}

void
V2vConsumer::requestPositionStatus(Position target)
{
  m_scheduler.schedule(0_s, [this, target] () { this->scheduledRequest(target); });
}

void
V2vConsumer::scheduledRequest(Position target)
{
  // Interest name format
  // /v2vSafety/[targetVector]/[targetTimePointIsoString]/[LimitNumber]

  if (m_requestInProgress) {
    NDN_LOG_DEBUG("Already requested adjustments; ignoring the second request");
    // return;
  }

  auto position = m_positionGetter->getPosition();
  auto velocity = m_positionGetter->getSpeed();

  // In this section, we have made the target position coarser.
  // To be more specific, coordinates will only be multiple of 10.
  // If the regex of x or y coordinate is (a)+b.c,
  // Then, if b > 5, then make b=10,otherwise make b = 0
  // For example, a coordinate of 488.4 will be converted into 490
  // whereas 483.1 will be converted into 480

  double rem;
  rem = (int)target.x % 100;

  if(rem >= 50.0) {
    target.x = (int)target.x + (100.0-rem);
  }
  else {
    target.x = (int)target.x - rem;
  }
    
  rem = (int)target.y % 100;
  if(rem >= 50.0) {
    target.y = (int)target.y + (100.0-rem);
  }
  else {
    target.y = (int)target.y - rem;
  }
  std::cout<<"coarse target: " << target <<std::endl;
  auto distance = target - position;
  
  /*if (distance.x * velocity.x < 0 ||
      distance.y * velocity.y < 0 ||
      distance.z * velocity.z < 0) {
    // unrechable, as going different directions
    NDN_LOG_DEBUG("Target unreachable, going different directions. IGNORING");
    return;
  }

  if ((std::abs(distance.x) > 0.1 && std::abs(velocity.x) < 0.01) ||
      (std::abs(distance.y) > 0.1 && std::abs(velocity.y) < 0.01) ||
      (std::abs(distance.z) > 0.1 && std::abs(velocity.z) < 0.01)
      ) {
    // target unrechable
    NDN_LOG_DEBUG("distance_x: "<< std::abs(distance.x) <<" velocity: " << std::abs(velocity.x));
      NDN_LOG_DEBUG("distance_y: "<< std::abs(distance.y) <<" velocity: " << std::abs(velocity.y));
      NDN_LOG_DEBUG("distance_z: "<< std::abs(distance.z) <<" velocity: " << std::abs(velocity.z));
    NDN_LOG_DEBUG("Target unreachable. IGNORING");
    return;
  }*/

  auto maxSpeed = max(abs(velocity));
  if (maxSpeed < 0.1) {
    // speed is too low, ignore adjustments even requested
    NDN_LOG_DEBUG("Requested position status, but speed is too low for that. IGNORING");
    return;
  }

  auto maxDistance = max(abs(distance));
  if (maxDistance < 20) {
    // distance is too low, no point of adjustments even if requested
    NDN_LOG_DEBUG("Requested position status, but target is within 20 meters. IGNORING");
    return;
  }

  // distance / velocity

  double dvAngle = std::abs(angle(distance, velocity));
  if (dvAngle > 1) {
    NDN_LOG_DEBUG("Velocity angle makes the target unreachable. INGORING");
    return;
  }

  // rest is estimation, assuming target reachable with the current velocity

  using SecondsDouble = boost::chrono::duration<double>;

  auto absDistance = position.getDistance(target);
  auto absSpeed = velocity.getAbsSpeed();

  auto time = (absDistance / absSpeed);
    
  auto expectToBeAtTarget = time::system_clock::now() +
    time::duration_cast<time::nanoseconds>(SecondsDouble(time));

    
  // Here, we make the expected arrival time coarses.
  // We are converting the time into complete integers with no fraction (i.e., 25 seconds, 42 seconds etc.).

  auto a = time::toUnixTimestamp(expectToBeAtTarget).count();
    //std::cout<<"seconds (): "<<a<<std::endl;
  a = a - (a%1000);
  //std::cout<<"seconds: "<<a<<std::endl;
    if(a < 0)
        std::cout<<a<<std::endl;
  expectToBeAtTarget = time::fromUnixTimestamp(time::milliseconds(a));

  Name request("/v2vSafety");
  request
    .append(name::Component(target.wireEncode()))
    //.append(name::Component(position.wireEncode()))
    .append(time::toIsoString(expectToBeAtTarget))
    .appendNumber(100);

  m_requestInProgress = true;

  Interest i(request);
  i.setCanBePrefix(false);
  i.setMustBeFresh(true);
  i.setInterestLifetime(1_s);

  ++m_interestCounter;
  m_face.expressInterest(i,
                         [this, counter=m_interestCounter, backThere=time::system_clock::now()] (const Interest& i, const Data& d) {
                           // data
                           m_requestInProgress = false;

                           // if real, do validation

                           int hopCount = 0;
                           auto hopCountTag = d.template getTag<lp::HopCountTag>();
                           if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
                             hopCount = *hopCountTag;
                           }
                           this->afterDataReceived(counter, time::system_clock::now() - backThere, hopCount);
                           //std::cerr << "get Data" <<std::endl;
                           NDN_LOG_DEBUG("Get Data");
                           this->m_doesRequireAdjustment = true;
                         },
                         [this, target] (const Interest&, const lp::Nack&) {
                           // nack
                           m_requestInProgress = false;
                           m_scheduler.schedule(200_ms, [this, target] () { this->scheduledRequest(target); });
                         },
                         [this, target] (const Interest&) {
                           // timeout
                           m_requestInProgress = false;
                           //this->m_doesRequireAdjustment = false;
                           NDN_LOG_DEBUG("Did not get Data");
                           //std::cerr <<"Did not get Data" <<std::endl;
                           m_scheduler.schedule(200_ms, [this, target] () { this->scheduledRequest(target); });
                         });
}

} // namespace ndn
