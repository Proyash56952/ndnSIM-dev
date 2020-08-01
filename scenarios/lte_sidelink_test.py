from lte_sidelink_base import *

c1 = addNode("car1")
c2 = addNode("car2")
c3 = addNode("car3")

def printNode(node):
    print("Node", node.node.GetId(), "(" + Names.FindName(node.node) + ")", node.dev, str(node.ip))

printNode(c1)
printNode(c2)
printNode(c3)


app1 = ndn.AppHelper("ns3::ndn::ConsumerCbr")
apps = app1.Install(c1.node)
apps.Start(Seconds(10.2))
apps.Stop(Seconds(20.1))

app2 = ndn.AppHelper("ndn::v2v::Producer")
apps = app2.Install(c3.node)
apps.Start(Seconds(5.1))


# #In this loop, we call an event runSumo() for each second. runSumo will use tracy to simulate a sumo scenario, and extract output for a particular second and write those into a trace file. Finally, using our customized mobility helper, mobility will be installed in nodes.
# for i in range(0,int(duration)):
#     ns.core.Simulator.Schedule(ns.core.Seconds(i), runSumo, wifiNodes,i)

# consumerHelper = ndn.AppHelper("ns3::ndn::ConsumerCbr")
# consumerHelper.SetPrefix("/v2safety/8thStreet/0,0,0/700,0,0/100")
# consumerHelper.SetAttribute("Frequency", StringValue("1"))
# consumerHelper.Install(wifiNodes.Get(0))

# producerHelper = ndn.AppHelper("ns3::ndn::Producer")
# producerHelper.SetPrefix("/v2safety/8thStreet")
# producerHelper.SetAttribute("PayloadSize", StringValue("50"))
# producerHelper.Install(wifiNodes.Get(nodeNum-1))

Simulator.Stop(cmd.duration)
Simulator.Run()
