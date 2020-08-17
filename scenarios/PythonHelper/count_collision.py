import os,sys
import csv
import subprocess
import re



data_file = open('results/collision_count.csv', 'w')
csv_writer = csv.writer(data_file)
csv_writer.writerow(["Duration","Total_Number_Of_Collision","Total_Collided_Vehicles"])


for duration in range(10,190,10):
    collisionCount = 0
    collidedVehicles = []
    path = 'python3 count_speed_adjustment.py --duration='+str(duration)+' 2>> temp/collision.txt'
    os.system(path)
    with open('temp/collision.txt') as stream:
        for line in stream:
            if 'collision' in line:
                print(line)
                collisionCount = collisionCount+1
                for m in re.finditer("'f",line):
                    #i = re.finditer("f",line).start()
                    #print(m.start())
                    pos = m.start()+2
                    v = line[pos:pos+3]
                    print(v)
                    collidedVehicles.append(v)
    
    unique_vehicles = set(collidedVehicles)
    print(unique_vehicles)
    csv_writer.writerow([duration, collisionCount,len(unique_vehicles)])
    os.system("rm temp/collision.txt")
