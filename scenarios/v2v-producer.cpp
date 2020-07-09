/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-producer.hpp"

#include <ndn-cxx/interest.hpp>

namespace ndn {

V2vProducer::V2vProducer(const std::string& id,
                         std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain)
  : m_id(id)
  , m_positionGetter(positionGetter)
  , m_keyChain(keyChain)
  // , m_scheduler(m_face.getIoService())
{
  std::cerr << "[" << time::system_clock::now() << "] starting producer" << std::endl;

  // // register prefix and set interest filter on producer face
  // m_face.setInterestFilter("/hello", std::bind(&RealApp::respond, this, _2),
  //                          std::bind([]{}), std::bind([]{}));

  // m_scheduler.schedule(time::seconds(2), [this] {
  //     m_face.expressInterest(Interest("/hello/world"),
  //                            std::bind([] { std::cout << "Hello!" << std::endl; }),
  //                            std::bind([] { std::cout << "NACK!" << std::endl; }),
  //                            std::bind([] { std::cout << "Bye!.." << std::endl; }));
  //   });
}

void
V2vProducer::respondToAnyInterest(const Interest& interest)
{
  // Data data(interest.getName());
  // m_keyChain.sign(data);
  // m_face.put(data);
}

} // namespace ndn
