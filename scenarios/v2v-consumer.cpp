/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-consumer.hpp"

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/logger.hpp>

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
  scheduleNext();
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
  // /v2vSafety/[targetVector]/[sourceVector]/[targetTimePointIsoString]/[LimitNumber]

  if (m_requestInProgress) {
    NDN_LOG_DEBUG("Already requested adjustments; ignoring the second request");
    return;
  }

  auto position = m_positionGetter->getPosition();
  auto velocity = m_positionGetter->getSpeed();


  auto distance = target - position;

  std::cerr << "Position: " << distance << std::endl;
  std::cerr << "Velocity: " << velocity << std::endl;

  if (distance.x * velocity.x < 0 ||
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
    NDN_LOG_DEBUG("Target unreachable. IGNORING");
    return;
  }

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

  auto time = absDistance / absSpeed;
  auto expectToBeAtTarget = time::system_clock::now() +
    time::duration_cast<time::nanoseconds>(SecondsDouble(time));

  Name request("/v2vSafety");
  request
    .append(name::Component(target.wireEncode()))
    .append(name::Component(position.wireEncode()))
    .append(time::toIsoString(expectToBeAtTarget))
    .appendNumber(100);
    
  m_requestInProgress = true;
  //auto t = target.wireEncode();
  //std::cout<< wireDecode(t);
  Interest i(request);
  i.setCanBePrefix(false);
  i.setMustBeFresh(true);
  i.setInterestLifetime(1_s);

  m_face.expressInterest(i,
                         [this] (const Interest&, const Data&) {
                           // data
                           m_requestInProgress = false;

                           // if real, do validation

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
                           m_scheduler.schedule(200_ms, [this, target] () { this->scheduledRequest(target); });
                         });
}

} // namespace ndn
