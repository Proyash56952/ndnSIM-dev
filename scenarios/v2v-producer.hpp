/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NDNSIM_SCENARIOS_V2V_PRODUCER_HPP
#define NDNSIM_SCENARIOS_V2V_PRODUCER_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include "v2v-position-getter.hpp"

namespace ndn {

class V2vProducer
{
public:
  static const char*
  getClassName()
  {
    static std::string name = "ndn::v2v::Producer";
    return name.c_str();
  }

  V2vProducer(const std::string& id,
              std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain);

  void
  run()
  {
    // do nothing
  }

  bool
  doesRequireAdjustment()
  {
    BOOST_ASSERT(false);
    return false;
  }

  void
  requestPositionStatus(Position position)
  {
    //BOOST_ASSERT(false);
  }

private:
  void
  respondIfCrashEstimate(const Interest& interest);

private:
  std::string m_id;
  std::shared_ptr<PositionGetter> m_positionGetter;
  KeyChain& m_keyChain;
  Face m_face;
  // Scheduler m_scheduler;
};

} // namespace ndn

#endif // NDNSIM_SCENARIOS_V2V_PRODUCER_HPP
