/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-position-getter.hpp"

namespace ndn {

BOOST_CONCEPT_ASSERT((WireEncodable<Vector>));
BOOST_CONCEPT_ASSERT((WireEncodableWithEncodingBuffer<Vector>));
BOOST_CONCEPT_ASSERT((WireDecodable<Vector>));

uint32_t CoordinateType = 1111;

template<encoding::Tag TAG>
size_t
Vector::wireEncode(EncodingImpl<TAG>& encoder) const
{
  // Vector ::= GENERIC-NAME-COMPONENT-TYPE TLV-LENGTH
  //               Coordinate Coordinate Coordinate # x y z
  //
  // Coordinate ::= COORDINATE-TYPE TLV-LENGTH
  //                  DoubleValue

  size_t totalLength = 0;

  totalLength += prependDoubleBlock(encoder, CoordinateType, z);
  totalLength += prependDoubleBlock(encoder, CoordinateType, y);
  totalLength += prependDoubleBlock(encoder, CoordinateType, x);

  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::GenericNameComponent);
  return totalLength;
}

NDN_CXX_DEFINE_WIRE_ENCODE_INSTANTIATIONS(Vector);

Block
Vector::wireEncode() const
{
  EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  return buffer.block();
}

void
Vector::wireDecode(const Block& wire)
{
  wire.parse();

  // Vector ::= GENERIC-NAME-COMPONENT-TYPE TLV-LENGTH
  //               Coordinate Coordinate Coordinate # x y z
  //
  // Coordinate ::= COORDINATE-TYPE TLV-LENGTH
  //                  DoubleValue

  auto val = wire.elements_begin();

  auto readCoordinate = [&] () {
    if (val != wire.elements_end() && val->type() == CoordinateType) {
      auto value = encoding::readDouble(*val);
      ++val;
      return value;
    }
    else {
      NDN_THROW(tlv::Error("Invalid contents of the coordinate"));
    }
  };

  x = readCoordinate();
  y = readCoordinate();
  z = readCoordinate();
}

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

Position
Ns3PositionGetter::getCoarsedPosition()
{
  auto pos = m_mobility->GetPosition();
    double rem;
    rem = (int) pos.x % 5;
    if(rem > 2.0) {
      pos.x += (5.0-rem);
    }
    else {
      pos.x -= rem;
    }
    
    rem = (int)pos.y % 5;
    if(rem > 2.0) {
      pos.y += (5.0-rem);
    }
    else {
      pos.y -= rem;
    }
  return Position{pos.x, pos.y, pos.z};
}

Speed
Ns3PositionGetter::getSpeed()
{
  auto pos = m_mobility->GetVelocity();
  return Speed{pos.x, pos.y, pos.z};
}


} // namespace ndn
