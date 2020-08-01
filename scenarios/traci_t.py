import os,sys

if 'SUMO_HOME' in os.environ:
    tools = os.path.join(os.environ['SUMO_HOME'], 'tools')
    sys.path.append(tools)
    #sys.path.append(os.path.join(os.environ.get("SUMO_HOME"), 'tools'))
else:
    sys.exit("please declare environment variable 'SUMO_HOME'")
    
from lte_sidelink_base import *

import traci
import traci.constants as tc
import time
import re
import sumolib
import math

from ns.mobility import MobilityModel, ConstantVelocityMobilityModel

net = sumolib.net.readNet('src/ndnSIM/scenarios/sumo/intersection.net.xml')
sumoCmd = ["sumo", "-c", "src/ndnSIM/scenarios/sumo/intersection.sumocfg"]

traci.start(sumoCmd, label="dry-run") # whole run to estimate and created all nodes with out of bound position and 0 speeds
g_traciDryRun = traci.getConnection("dry-run")

traci.start(sumoCmd, label="step-by-step")
g_traciStepByStep = traci.getConnection("step-by-step")

g_names = {}
p_names = {}
vehicleList = []
prevList = []
nowList = []
posOutOfBound = Vector(0, 0, -2000)
departedCount = 0


def createAllVehicles(simTime):
    g_traciDryRun.simulationStep(simTime)
    #vehicleList = g_traciDryRun.simulation.getLoadedIDList()
    for vehicle in g_traciDryRun.simulation.getLoadedIDList():
        node = addNode(vehicle)
        #print(str(vehicle)+ "   "+str(node))
        g_names[vehicle] = node
        node.mobility = node.node.GetObject(ConstantVelocityMobilityModel.GetTypeId())
        node.mobility.SetPosition(posOutOfBound)
        node.mobility.SetVelocity(Vector(0, 0, 0))
        node.time = -1
        vehicleList.append(vehicle)
    #print(vehicleList[1])
    persons = g_traciDryRun.person.getIDList()
    for person in persons:
        node = addNode(person)
        #print(person)
        p_names[person] = node
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

    #print("Node [%s] change speed to [%s] (now = %f seconds); reference speed %f, current position: %s, target position: %s" % (node.name, str(estimatedSpeed), Simulator.Now().To(Time.S).GetDouble(), referenceSpeed, str(prevPos), str(targetPos)))
    node.mobility.SetVelocity(estimatedSpeed)

def prepositionNode(node, targetPos, currentSpeed, angle, targetTime):
    '''This one is trying to set initial position of the node in such a way so it will be
       traveling at currentSpeed and arrives at the targetPos (basically, set position using reverse speed)'''

    #print("Node [%s] will arrive at [%s] in %f seconds (now = %f seconds)" % (node.name, str(targetPos), targetTime, Simulator.Now().To(Time.S).GetDouble()))

    speed = Vector(currentSpeed * math.sin(angle * math.pi / 180), currentSpeed * math.cos(angle * math.pi / 180), 0.0)

    #print(str(speed), currentSpeed, angle, currentSpeed* math.cos(angle * math.pi / 180), currentSpeed * math.sin(angle * math.pi / 180))

    prevPos = Vector(targetPos.x + targetTime * -speed.x, targetPos.y + targetTime * -speed.y, 0)
    #print("          initially positioned at [%s] with speed [%s] (sumo speed %f)" % (str(prevPos), str(speed), currentSpeed))

    node.mobility.SetPosition(prevPos)
    node.mobility.SetVelocity(speed)



def getTargets(vehicle):
    pos = []

    # get the lane id in which the vehicle is currently on
    currentLaneId = g_traciStepByStep.vehicle.getLaneID(vehicle)
    currentLane = net.getLane(currentLaneId)

    x, y = sumolib.geomhelper.positionAtShapeOffset(currentLane.getShape(), currentLane.getLength())

    # Position at the end of the current lane
    pos.append(Vector(x, y, 0))

    for connection in currentLane.getOutgoing():
        nextLane = connection.getToLane()
        x, y = sumolib.geomhelper.positionAtShapeOffset(nextLane.getShape(), 0)
        pos.append(Vector(x, y, 0))

    return pos

def runSumoStep():
    Simulator.Schedule(Seconds(time_step), runSumoStep)

    nowTime = Simulator.Now().To(Time.S).GetDouble()
    targetTime = Simulator.Now().To(Time.S).GetDouble() + time_step

    g_traciStepByStep.simulationStep(Simulator.Now().To(Time.S).GetDouble() + time_step)

    print(nowTime, g_traciStepByStep.vehicle.getIDList())
    for vehicle in g_traciStepByStep.vehicle.getIDList():
        node = g_names[vehicle]

        pos = g_traciStepByStep.vehicle.getPosition(vehicle)
        speed = g_traciStepByStep.vehicle.getSpeed(vehicle)
        angle = g_traciStepByStep.vehicle.getAngle(vehicle)

        if node.time < 0: # a new node
            node.time = targetTime
            prepositionNode(node, Vector(pos[0], pos[1], 0.0), speed, angle, targetTime - nowTime)
            node.referencePos = Vector(pos[0], pos[1], 0.0)

            targets = getTargets(vehicle)
            print("          Points of interests:", [str(target) for target in targets])
        else:
            node.time = targetTime
            setSpeedToReachNextWaypoint(node, node.referencePos, Vector(pos[0], pos[1], 0.0), targetTime - nowTime, speed)
            node.referencePos = Vector(pos[0], pos[1], 0.0)
        #g_traciStepByStep.vehicle.setSpeedMode(vehicle,0)
        #g_traciStepByStep.vehicle.setMinGap(vehicle,0)
        
    for person in g_traciStepByStep.person.getIDList():
        node = p_names[person]

        pos = g_traciStepByStep.person.getPosition(person)
        speed = g_traciStepByStep.person.getSpeed(person)
        angle = g_traciStepByStep.person.getAngle(person)

        if node.time < 0: # a new node
            node.time = targetTime
            prepositionNode(node, Vector(pos[0], pos[1], 0.0), speed, angle, targetTime - nowTime)
            node.referencePos = Vector(pos[0], pos[1], 0.0)

            #targets = getTargets(vehicle)
            #print("          Points of interests:", [str(target) for target in targets])
        else:
            node.time = targetTime
            setSpeedToReachNextWaypoint(node, node.referencePos, Vector(pos[0], pos[1], 0.0), targetTime - nowTime, speed)
            node.referencePos = Vector(pos[0], pos[1], 0.0)

def findPoint(x1, y1, x2,
          y2, x, y) :
    if (x > x1 and x < x2 and
        y > y1 and y < y2) :
        return True
    else :
        return False

def diff(li1, li2):
    return (list(set(li1) - set(li2)))

passingVehicle_step = 0.5

def passingVehicle():
    Simulator.Schedule(Seconds(passingVehicle_step), passingVehicle)
    nowTime = Simulator.Now().To(Time.S).GetDouble()
    #print("hola "+str(nowTime))
    
    for vehicle in vehicleList:
        node = g_names[vehicle]
        position = node.mobility.GetPosition()
        #print(position)
        if (findPoint(485,485,515,515,position.x,position.y)):
            #print(position)
            nowList.append(vehicle)
    global prevList, departedCount
    departList = diff(prevList,nowList)
    #print(nowTime)
    #print(prevList)
    #print(nowList)
    #print(departList)
    prevList = nowList[:]
    nowList.clear()
    #print(nowList)
    
    #print(len(departList))
    departedCount = departedCount + len(departList)
    print(departedCount)

# this will only be the case when a car is moving in a fixed velocity and then try to adjust a 2m space from it's original reaching point
def actionOnInterest(vehID):
    node = g_names[vehID]
    g_traciStepByStep.simulationStep(Simulator.Now().To(Time.S).GetDouble())
    oldSpeed = g_traciStepByStep.vehicle.getSpeed(vehID)
    newSpeed = oldSpeed - 4
    g_traciStepByStep.vehicle.slowDown(vehID,newSpeed,1)

consumerAppHelper = ndn.AppHelper("ndn::v2v::Consumer")

def test():
    consumerNode = g_names["f2.0"]
    print(consumerNode.node)
    apps = consumerAppHelper.Install(consumerNode.node)
    apps.Start(Seconds(0.1))
    consumerNode.apps = apps.Get(0)

def test2():
    consumerNode = g_names["f2.0"]
    consumerNode.apps.SetAttribute("RequestPositionStatus", StringValue("486.4:495.2:0"))
    
createAllVehicles(cmd.duration.To(Time.S).GetDouble())

# consumerNode = g_names["f1.1"]
# print(consumerNode.node)

# consumerApp = ndn.AppHelper("ndn::v2v::Consumer")
# apps = consumerApp.Install(consumerNode.node)

test()
Simulator.Schedule(Seconds(5.1), test2)


Simulator.Schedule(Seconds(1), runSumoStep)

Simulator.Schedule(Seconds(1), passingVehicle)


Simulator.Stop(cmd.duration)
Simulator.Run()

g_traciStepByStep.close()
traci.close()

