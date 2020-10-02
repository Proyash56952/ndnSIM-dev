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

inline std::ostream&
operator<<(std::ostream& os, const Vector& v)
{
  os << "{x=" << v.x << ", y=" << v.y << ", z=" << v.z << "}";
  return os;
}

inline Vector
operator-(const Vector& lhs, const Vector& rhs)
{
  return Vector(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

inline Vector
operator+(const Vector& lhs, const Vector& rhs)
{
  return Vector(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

inline Vector
operator*(const Vector& lhs, const Vector& rhs)
{
  return Vector(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

inline Vector
operator*(const Vector& lhs, double rhs)
{
  return Vector(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

inline Vector
abs(const Vector& v)
{
  return {std::abs(v.x), std::abs(v.y), std::abs(v.z)};
}

inline double
max(const Vector& v)
{
  return std::max(std::max(v.x, v.y), v.z);
}

inline double
dotProduct(const Vector& v1, const Vector& v2)
{
  auto v = v1 * v2;
  return v.x + v.y + v.z;
}

inline double
length(const Vector& v)
{
  return std::sqrt(std::pow(v.x, 2) + std::pow(v.y, 2) + std::pow(v.z, 2));
}

inline double
angle(const Vector& v1, const Vector& v2)
{
  return acos(dotProduct(v1, v2) / (length(v1) + length(v2)));
}

NDN_CXX_DECLARE_WIRE_ENCODE_INSTANTIATIONS(Vector);

struct Position : public Vector
{
  using Vector::Vector;

  Position(const Vector& v)
    : Vector(v)
  {
  }

  Position(Vector&& v)
    : Vector(std::move(v))
  {
  }

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

  Speed(const Vector& v)
    : Vector(v)
  {
  }

  Speed(Vector&& v)
    : Vector(std::move(v))
  {
  }

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
    
  virtual Position
  getCoarsedPosition() = 0;

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
    
  Position
  getCoarsedPosition() override;

  Speed
  getSpeed() override;

private:
  ns3::Ptr<ns3::MobilityModel> m_mobility;
};

} // namespace ndn

#endif // NDNSIM_SCENARIOS_V2V_POSITION_GETTER_HPP
