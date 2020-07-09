/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP
#define NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP

#include <iostream>

namespace ndn {

struct Position
{
  double x;
  double y;
  double z;
};

inline std::ostream&
operator<<(std::ostream& os, const Position& pos)
{
  os << "{x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << "}";
  return os;
}

class PositionGetter
{
public:
  virtual Position
  getPosition() = 0;

  virtual
  ~PositionGetter() = default;
};

} // namespace ndn

#include <ns3/node.h>
#include <ns3/mobility-model.h>

namespace ndn {

class Ns3PositionGetter : public PositionGetter
{
public:
  Ns3PositionGetter(ns3::Ptr<ns3::Node> selfNode);

  Position
  getPosition() override;

private:
  ns3::Ptr<ns3::MobilityModel> m_mobility;
};

} // namespace ndn

#endif // NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP
