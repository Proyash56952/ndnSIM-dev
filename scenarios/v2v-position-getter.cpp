/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-position-getter.hpp"

namespace ndn {

Ns3PositionGetter::Ns3PositionGetter(ns3::Ptr<ns3::Node> selfNode)
{
  m_mobility = selfNode->GetObject<ns3::MobilityModel>();
}

Position
Ns3PositionGetter::getPosition()
{
  auto pos = m_mobility->GetPosition();
  return Position{pos.x, pos.y, pos.z};
}

} // namespace ndn
