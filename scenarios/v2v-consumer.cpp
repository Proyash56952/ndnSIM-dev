/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-consumer.hpp"

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/logger.hpp>

namespace ndn {

NDN_LOG_INIT(v2v.Consumer);

V2vConsumer::V2vConsumer(const std::string& id, std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain)
  : m_id(id)
  , m_positionGetter(positionGetter)
  , m_keyChain(keyChain)
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
  if (m_requestInProgress) {
    NDN_LOG_DEBUG("Already requested adjustments; ignoring the second request");
    return;
  }

  auto position = m_positionGetter->getPosition();
  auto velocity = m_positionGetter->getSpeed();

  // distance / velocity

  auto distance = position.getDistance(target);
  auto speed = velocity.getAbsSpeed();

  if (speed < 0.1) {
    // speed is too low, ignore adjustments even requested
    NDN_LOG_DEBUG("Requested position status, but speed is too low for that. IGNORING");
    return;
  }

  if (distance < 20) {
    // distance is too low, no point of adjustments even if requested
    NDN_LOG_DEBUG("Requested position status, but target is within 20 meters. IGNORING");
    return;
  }

  using SecondsDouble = boost::chrono::duration<double>;

  auto time = distance / speed;
  auto expectToBeAtTarget = time::system_clock::now() +
    time::duration_cast<time::nanoseconds>(SecondsDouble(time));

  Name request("/v2vSafety");
  request
    .append(name::Component(target.wireEncode()))
    .append(name::Component(position.wireEncode()))
    .append(time::toIsoString(expectToBeAtTarget))
    .appendNumber(100);

  m_requestInProgress = true;

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
