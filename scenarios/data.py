import os,sys
from lte_sidelink_base import *

import time
import re
import math
import csv

from ns.mobility import MobilityModel, ConstantVelocityMobilityModel


g_names = {}
p_names = {}
vehicleList = []
prevList = []
nowList = []
posOutOfBound = Vector(0, 0, -2000)
departedCount = 0

c1 = addNode("car1")
c2 = addNode("car2")
c3 = addNode("car3")

consumerAppHelper = ndn.AppHelper("ndn::v2v::Consumer")
producerAppHelper = ndn.AppHelper("ndn::v2v::Producer")

c1.mobility = c1.node.GetObject(ConstantVelocityMobilityModel.GetTypeId())
c2.mobility = c2.node.GetObject(ConstantVelocityMobilityModel.GetTypeId())
c3.mobility = c3.node.GetObject(ConstantVelocityMobilityModel.GetTypeId())

c1.mobility.SetPosition(Vector(0,0,0))
c2.mobility.SetPosition(Vector(50,0,0))
c3.mobility.SetPosition(Vector(100,0,0))

apps = consumerAppHelper.Install(c1.node)
apps.Start(Seconds(0.1))
c1.apps = apps.Get(0)

proapps = producerAppHelper.Install(c3.node)
proapps.Start(Seconds(0.5))
c3.proapps = proapps.Get(0)

def sendInterest():
    c1.apps.SetAttribute("RequestPositionStatus", StringValue("100.0:0.0:0.0"))

def getValue():
    Simulator.Schedule(Seconds(1), getValue)
    requireAdjustment = BooleanValue()
    c1.apps.GetAttribute("DoesRequireAdjustment",requireAdjustment)
    print(requireAdjustment)

Simulator.Schedule(Seconds(3.1), sendInterest)
Simulator.Schedule(Seconds(1), getValue)



Simulator.Stop(cmd.duration)
Simulator.Run()


