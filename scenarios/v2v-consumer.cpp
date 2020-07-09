/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-consumer.hpp"

#include <ndn-cxx/interest.hpp>

namespace ndn {

V2vConsumer::V2vConsumer(const std::string& id, std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain)
  : m_id(id)
  , m_positionGetter(positionGetter)
  , m_keyChain(keyChain)
  , m_scheduler(m_face.getIoService())
{
  std::cerr << "[" << time::system_clock::now() << "] starting consumer" << std::endl;

  // // register prefix and set interest filter on producer face
  // m_face.setInterestFilter("/hello", std::bind(&RealApp::respond, this, _2),
  //                          std::bind([]{}), std::bind([]{}));

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

} // namespace ndn
