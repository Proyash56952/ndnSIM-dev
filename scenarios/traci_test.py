from lte_sidelink_base import *

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
posOutOfBound = Vector(0, 0, -2000)


def createAllVehicles(simTime):
    g_traciDryRun.simulationStep(simTime)
    for vehicle in g_traciDryRun.simulation.getLoadedIDList():
        node = addNode(vehicle)
        g_names[vehicle] = node
        node.mobility = node.node.GetObject(ConstantVelocityMobilityModel.GetTypeId())
        node.mobility.SetPosition(posOutOfBound)
        node.mobility.SetVelocity(Vector(0, 0, 0))
        node.time = -1
    g_traciDryRun.close()

time_step = 1

def setSpeedToReachNextWaypoint(node, referencePos, targetPos, targetTime, referenceSpeed):
    prevPos = node.mobility.GetPosition()
    if prevPos.z < 0:
        raise RuntimeError("Can only calculate waypoint speed for the initially positioned nodes")

    # prevPos and referencePos need to be reasonably similar
    error = Vector(abs(prevPos.x - referencePos.x), abs(prevPos.y - referencePos.y), 0)
    if error.x > 0.1 or error.y > 0.1:
        print(">> Node [%s] Position %s, expected to be at %s" % (node.name, prevPos, referencePos))
        print(">> Node [%s] Position error: %s (now = %f seconds)" % (node.name, str(error), Simulator.Now().To(Time.S).GetDouble()))
        raise RuntimeError("Something off with node positioning; reference and actual current position differ over 10cm")

    distance = Vector(targetPos.x - prevPos.x, targetPos.y - prevPos.y, 0)

    distanceActual = math.sqrt(math.pow(distance.x, 2) + pow(distance.y, 2))
    estimatedSpeedActual = distanceActual / targetTime

    estimatedSpeed = Vector(distance.x / targetTime, distance.y / targetTime, 0)

    print("Node [%s] change speed to [%s] (now = %f seconds); reference speed %f, current position: %s, target position: %s" % (node.name, str(estimatedSpeed), Simulator.Now().To(Time.S).GetDouble(), referenceSpeed, str(prevPos), str(targetPos)))
    node.mobility.SetVelocity(estimatedSpeed)

def prepositionNode(node, targetPos, currentSpeed, angle, targetTime):
    '''This one is trying to set initial position of the node in such a way so it will be
       traveling at currentSpeed and arrives at the targetPos (basically, set position using reverse speed)'''

    print("Node [%s] will arrive at [%s] in %f seconds (now = %f seconds)" % (node.name, str(targetPos), targetTime, Simulator.Now().To(Time.S).GetDouble()))

    speed = Vector(currentSpeed * math.sin(angle * math.pi / 180), currentSpeed * math.cos(angle * math.pi / 180), 0.0)

    # print(str(speed), currentSpeed, angle, currentSpeed* math.cos(angle * math.pi / 180), currentSpeed * math.sin(angle * math.pi / 180))

    prevPos = Vector(targetPos.x + targetTime * -speed.x, targetPos.y + targetTime * -speed.y, 0)
    print("          initially positioned at [%s] with speed [%s] (sumo speed %f)" % (str(prevPos), str(speed), currentSpeed))

    node.mobility.SetPosition(prevPos)
    node.mobility.SetVelocity(speed)

def runSumoStep():
    Simulator.Schedule(Seconds(time_step), runSumoStep)

    nowTime = Simulator.Now().To(Time.S).GetDouble()
    targetTime = Simulator.Now().To(Time.S).GetDouble() + time_step

    # print(Simulator.Now().To(Time.S).GetDouble() + time_step)
    g_traciStepByStep.simulationStep(Simulator.Now().To(Time.S).GetDouble() + time_step)

    for vehicle in g_traciStepByStep.vehicle.getIDList():
        node = g_names[vehicle]

        pos = g_traciStepByStep.vehicle.getPosition(vehicle)
        speed = g_traciStepByStep.vehicle.getSpeed(vehicle)
        angle = g_traciStepByStep.vehicle.getAngle(vehicle)

        if node.time < 0: # a new node
            node.time = targetTime
            prepositionNode(node, Vector(pos[0], pos[1], 0.0), speed, angle, targetTime - nowTime)
            node.referencePos = Vector(pos[0], pos[1], 0.0)
        else:
            node.time = targetTime
            setSpeedToReachNextWaypoint(node, node.referencePos, Vector(pos[0], pos[1], 0.0), targetTime - nowTime, speed)
            node.referencePos = Vector(pos[0], pos[1], 0.0)

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
