/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"
#include "ns3/node.h"
#include "ns3/names.h"

#include "v2v-position-getter.hpp"

namespace ns3 {

// Class inheriting from ns3::Application
template<class RealApp>
class RealAppHelper : public Application
{
public:
  static TypeId
  GetTypeId()
  {
    static TypeId tid = TypeId(RealApp::getClassName())
      .SetParent<Application>()
      .AddConstructor<RealAppHelper<RealApp>>();

    return tid;
  }

protected:
  // inherited from Application base class.
  virtual void
  StartApplication()
  {
    auto positionGetter = std::make_shared<::ndn::Ns3PositionGetter>(GetNode());
    std::string id = Names::FindName(GetNode());

    // Create an instance of the app, and passing the dummy version of KeyChain (no real signing)
    m_instance = ::ndn::make_unique<RealApp>(id, positionGetter, ndn::StackHelper::getKeyChain());
    m_instance->run(); // can be omitted
  }

  virtual void
  StopApplication()
  {
    // Stop and destroy the instance of the app
    m_instance.reset();
  }

private:
  std::unique_ptr<RealApp> m_instance;
};

} // namespace ns3

#include "v2v-producer.hpp"
#include "v2v-consumer.hpp"

namespace {
using App1 = ns3::RealAppHelper<ndn::V2vConsumer>;
NS_OBJECT_ENSURE_REGISTERED(App1);

using App2 = ns3::RealAppHelper<ndn::V2vProducer>;
NS_OBJECT_ENSURE_REGISTERED(App2);
}
