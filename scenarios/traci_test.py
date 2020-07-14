from lte_sidelink_base import *

# c1 = addNode("car1")
# c2 = addNode("car2")
# c3 = addNode("car3")

# def printNode(node):
#     print("Node", node.node.GetId(), "(" + Names.FindName(node.node) + ")", node.dev, str(node.ip))

# printNode(c1)
# printNode(c2)
# printNode(c3)


# app1 = ndn.AppHelper("ndn::v2v::Consumer")
# apps = app1.Install(c1.node)
# apps.Start(Seconds(10.2))
# apps.Stop(Seconds(20.1))

# app2 = ndn.AppHelper("ndn::v2v::Producer")
# apps = app2.Install(c3.node)
# apps.Start(Seconds(5.1))


# # #In this loop, we call an event runSumo() for each second. runSumo will use tracy to simulate a sumo scenario, and extract output for a particular second and write those into a trace file. Finally, using our customized mobility helper, mobility will be installed in nodes.
# # for i in range(0,int(duration)):
# #     ns.core.Simulator.Schedule(ns.core.Seconds(i), runSumo, wifiNodes,i)

# # consumerHelper = ndn.AppHelper("ns3::ndn::ConsumerCbr")
# # consumerHelper.SetPrefix("/v2safety/8thStreet/0,0,0/700,0,0/100")
# # consumerHelper.SetAttribute("Frequency", StringValue("1"))
# # consumerHelper.Install(wifiNodes.Get(0))

# # producerHelper = ndn.AppHelper("ns3::ndn::Producer")
# # producerHelper.SetPrefix("/v2safety/8thStreet")
# # producerHelper.SetAttribute("PayloadSize", StringValue("50"))
# # producerHelper.Install(wifiNodes.Get(nodeNum-1))

# Simulator.Stop(cmd.duration)
# Simulator.Run()


import traci
import traci.constants as tc
import time
import re
import sumolib
import math

from ns.mobility import MobilityModel, ConstantVelocityMobilityModel

net = sumolib.net.readNet('sumo/intersection.net.xml')
sumoCmd = ["sumo", "-c", "sumo/intersection.sumocfg"]

traci.start(sumoCmd, label="dry-run") # whole run to estimate and created all nodes with out of bound position and 0 speeds
g_traciDryRun = traci.getConnection("dry-run")

traci.start(sumoCmd, label="step-by-step")
g_traciStepByStep = traci.getConnection("step-by-step")

g_names = {}
posOutOfBound = Vector(-0, -0, 0)


def createAllVehicles(simTime):
    g_traciDryRun.simulationStep(simTime)
    for vehicle in g_traciDryRun.simulation.getLoadedIDList():
        node = addNode(vehicle)
        g_names[vehicle] = node
        node.mobility = node.node.GetObject(ConstantVelocityMobilityModel.GetTypeId())
        node.mobility.SetPosition(posOutOfBound)
        node.mobility.SetVelocity(Vector(0, 0, 0))
    g_traciDryRun.close()

time_step = 0.5
    
def runSumoStep():
    Simulator.Schedule(Seconds(time_step), runSumoStep)
    
    g_traciStepByStep.simulationStep(Simulator.Now().To(Time.S).GetDouble() + time_step)

    for vehicle in g_traciStepByStep.vehicle.getIDList():
    # for vehicle in g_traciStepByStep.simulation.getLoadedIDList(): # these are only the new nodes, no re-routes
        node = g_names[vehicle]
        
        speedTraci = g_traciStepByStep.vehicle.getSpeed(vehicle)
        angleTraci = g_traciStepByStep.vehicle.getAngle(vehicle)
        posTraci = g_traciStepByStep.vehicle.getPosition(vehicle)

        pos = Vector(posTraci[0], posTraci[1], 0.0)
        speed = Vector(speedTraci * math.cos(angleTraci * math.pi / 180),
                       speedTraci * math.sin(angleTraci * math.pi / 180), 0.0)

        print(vehicle, "pos =", pos, " speed = (%f)" % speedTraci, speed, "angle = ", angleTraci)

        node.mobility.SetPosition(pos)
        node.mobility.SetVelocity(speed)
        
        # get the lane id in which the vehicle is currently on
        # lane = g_traciStepByStep.vehicle.getLaneID(vehicles[i])
        #print("lane: "+lane+" "+vehicles[i])
        
        # if(lane == "1i_3"):
        # #get the lane object using sumolib.net
        #     lane_ = net.getLane(lane)
            
        #     # retrieve the list of outgoing lanes from current lane
        #     outGoing = lane_.getOutgoing()
            
        #     if(len(outGoing) > 0):
        #         nextLaneId = outGoing[0].getToLane().getID() #get the next lane the vehilce will visit
        #         #laneLength = outGoing[0].getToLane().getLength()
        #         print("next lane will be : "+nextLaneId)
        #         #print("length: "+str(laneLength))
            

        #     x,y = sumolib.geomhelper.positionAtShapeOffset(net.getLane(nextLaneId).getShape(), 0)
        #     print("The position will be : " +str(x)+"  " +str(y))
            
    # f.close()


createAllVehicles(cmd.duration.To(Time.S).GetDouble())

Simulator.Schedule(Seconds(1), runSumoStep)

Simulator.Stop(cmd.duration)
Simulator.Run()

g_traciStepByStep.close()
traci.close()
