import os,sys
import csv
import subprocess
import re

collisionCount = 0
collidedVehicles = []

data_file = open('results/collision_details.csv', 'w')
csv_writer = csv.writer(data_file)
csv_writer.writerow(["Number_Of_Collision","Time","Vehicle1", "Vehicle2"])

path = 'python3 count_speed_adjustment.py --duration=70 2>> temp/collision_details.txt'
os.system(path)
with open('temp/collision_details.txt') as stream:
    for line in stream:
        if 'collision' in line:
            collisionCount = collisionCount+1
            for m in re.finditer("'f",line):
                #i = re.finditer("f",line).start()
                #print(m.start())
                pos = m.start()+2
                v = line[pos:pos+3]
                print(v)
                collidedVehicles.append(v)
            timeStart = re.search("time=",line).start() + 5
            timeEnd = re.search("stage",line).start() - 1
            time = line[timeStart:timeEnd]
            print(time)
            csv_writer.writerow([collisionCount,str(time),str(collidedVehicles[0]),str(collidedVehicles[1])])
            collidedVehicles.clear()
                
os.system("rm temp/collision_details.txt")
