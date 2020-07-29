/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NDNSIM_SCENARIOS_V2V_CONSUMER_HPP
#define NDNSIM_SCENARIOS_V2V_CONSUMER_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include "v2v-position-getter.hpp"

namespace ndn {

class V2vConsumer
{
public:
  static const char*
  getClassName()
  {
    static std::string name = "ndn::v2v::Consumer";
    return name.c_str();
  }

  V2vConsumer(const std::string& id, std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain);

  ~V2vConsumer();

  void
  run()
  {
    // do nothing
  }

  bool
  doesRequireAdjustment() const
  {
    return m_doesRequireAdjustment;
  }

  void
  requestPositionStatus(Position position);

private:
  void
  scheduleNext();

  void
  scheduledRequest(Position target);

private:
  const std::string m_id;
  std::shared_ptr<PositionGetter> m_positionGetter;
  KeyChain& m_keyChain;
  Face m_face;
  Scheduler m_scheduler;

  bool m_requestInProgress = false;
  bool m_doesRequireAdjustment = false;
};

} // namespace ndn

#endif // NDNSIM_SCENARIOS_V2V_CONSUMER_HPP
