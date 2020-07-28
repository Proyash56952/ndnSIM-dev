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

  using SecondsDouble = boost::chrono::duration<double>;

  auto time = distance / speed;
  auto expectToBeAtTarget = time::system_clock::now() + SecondsDouble(time);

  Name request("/v2v");
}


} // namespace ndn
