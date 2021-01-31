/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "v2v-producer.hpp"

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/logger.hpp>


namespace ndn {

NDN_LOG_INIT(v2v.Producer);

V2vProducer::V2vProducer(const std::string& id,
                         std::shared_ptr<PositionGetter> positionGetter, KeyChain& keyChain)
  : m_id(id)
  , m_positionGetter(positionGetter)
  , m_keyChain(keyChain)
  // , m_scheduler(m_face.getIoService())
{
  std::cerr << "[" << time::system_clock::now() << "] starting producer" << std::endl;

  // register prefix and set interest filter on producer face
  m_face.setInterestFilter("/v2vSafety",
                           std::bind(&V2vProducer::respondIfCrashEstimate, this, _2),
                           std::bind([]{}),
                           std::bind([]{}));

  // m_scheduler.schedule(time::seconds(2), [this] {
  //     m_face.expressInterest(Interest("/hello/world"),
  //                            std::bind([] { std::cout << "Hello!" << std::endl; }),
  //                            std::bind([] { std::cout << "NACK!" << std::endl; }),
  //                            std::bind([] { std::cout << "Bye!.." << std::endl; }));
  //   });
}


void
V2vProducer::respondIfCrashEstimate(const Interest& interest)
{
  try {
    // Interest name format
    // /v2vSafety/[targetVector]/[targetTimePointIsoString]/[LimitNumber]

    auto position = m_positionGetter->getPosition();
    auto velocity = m_positionGetter->getSpeed();

    auto expectToBeAtTarget = time::fromIsoString(interest.getName()[-2].toUri());
    //Position source(interest.getName()[-3]);
    Position target(interest.getName()[-3]);

    /*if (source.getDistance(position) < 2) { // may need to check if this should be adjusted
      // don't respond
      NDN_LOG_TRACE("Too close to the source; not responding");
      return;
    }*/

    using SecondsDouble = boost::chrono::duration<double>;

    auto time = time::duration_cast<SecondsDouble>(expectToBeAtTarget - time::system_clock::now()).count();
    Position expectedPosition = position + (velocity * time);
    
    /*double rem;
    rem = (int)expectedPosition.x % 5;

    if(rem > 2.0) {
      expectedPosition.x = (int)expectedPosition.x + (5.0-rem);
    }
    else {
      expectedPosition.x = (int)expectedPosition.x - rem;
    }
        
    rem = (int)expectedPosition.y % 5;
    if(rem > 2.0) {
      expectedPosition.y = (int)expectedPosition.y + (5.0-rem);
    }
    else {
      expectedPosition.y = (int)expectedPosition.y - rem;
    }*/
      
    NDN_LOG_DEBUG(expectedPosition);
    NDN_LOG_DEBUG(expectedPosition.getDistance(target));
    if (expectedPosition.getDistance(target) < 6) { // within 6 meters
      NDN_LOG_DEBUG("Data will be sent");
      //std::cout<<"Data will be sent from expected position of "<<expectedPosition<<" and the target is: "<<target<<std::endl;
      Data data(interest.getName());
      data.setFreshnessPeriod(500_ms);

      Block content(tlv::ContentType);
      content.push_back(makeStringBlock(1, m_id));
      content.push_back(position.wireEncode());
      content.push_back(velocity.wireEncode());
      content.encode();
      data.setContent(content);

      m_keyChain.sign(data);
      m_face.put(data);
    }
    else {
      // most likely case; not expected at the position
      //std::cout<< "Got Interest, but don't expect to be at the position (" << target << "m from target" <<std::endl;
      NDN_LOG_TRACE("Got Interest, but don't expect to be at the position (" << target << "m from target");
    }
  }
  catch (const tlv::Error&) {
    // should not be possible in our case, but just in case
    NDN_LOG_DEBUG("Got interest, but cannot handle it");
      //std::cout<< "Got interest, but cannot handle it" << std::endl;
  }
}

} // namespace ndn
