import os,sys
import csv
import subprocess
import re

collisionCount = 0

data_file = open('results/collision_count.csv', 'w')
csv_writer = csv.writer(data_file)
csv_writer.writerow(["Duration","Total_Number_Of_Collision"])


for duration in range(10,31,10):
    path = 'python3 count_speed_adjustment.py --duration='+str(duration)+' 2>> temp/collision.txt'
    os.system(path)
    with open('temp/collision.txt') as stream:
        for line in stream:
            if 'collision' in line:
                print(line)
                collisionCount = collisionCount+1
    csv_writer.writerow([duration, collisionCount])
                    
                    
    os.system("rm temp/collision.txt")
