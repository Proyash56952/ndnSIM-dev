/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP
#define NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <iostream>
#include <cmath>

namespace ndn {

struct Vector
{
  Vector(double x, double y, double z)
    : x(x)
    , y(y)
    , z(z)
  {
  }

  Vector(const Block& wire)
  {
    wireDecode(wire);
  }

  template<encoding::Tag TAG>
  size_t
  wireEncode(EncodingImpl<TAG>& encoder) const;

  Block
  wireEncode() const;

  void
  wireDecode(const Block& wire);

  double x;
  double y;
  double z;
};

NDN_CXX_DECLARE_WIRE_ENCODE_INSTANTIATIONS(Vector);

struct Position : public Vector
{
  using Vector::Vector;

  double
  getDistance(const Position& rhs) const
  {
    return std::sqrt(std::pow(x - rhs.x, 2) + std::pow(y - rhs.y, 2) + std::pow(z - rhs.z, 2));
  }
};

inline std::ostream&
operator<<(std::ostream& os, const Position& pos)
{
  os << "{x=" << pos.x << "m, y=" << pos.y << "m, z=" << pos.z << "m}";
  return os;
}

struct Speed : public Vector
{
  using Vector::Vector;

  double
  getAbsSpeed() const
  {
    return std::sqrt(std::pow(x, 2) + std::pow(y, 2) + std::pow(z, 2));
  }
};

inline std::ostream&
operator<<(std::ostream& os, const Speed& pos)
{
  os << "{x=" << pos.x << "m/s, y=" << pos.y << "m/s, z=" << pos.z << "m/s}";
  return os;
}


class PositionGetter
{
public:
  virtual Position
  getPosition() = 0;

  virtual Speed
  getSpeed() = 0;

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

  Speed
  getSpeed() override;

private:
  ns3::Ptr<ns3::MobilityModel> m_mobility;
};

} // namespace ndn

#endif // NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP
