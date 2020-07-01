Please refer to http://ndnsim.net/examples.html (../docs/source/examples.rst) 
for detailed information about the examples.

### Steps to run simulations that uses TraCI and SUMO simulations 

To run this type of simulaion, you need to have SUMO installed in your machine. For TraCI, you need to build and compile SUMO from source code. In this [link](https://sumo.dlr.de/docs/Installing/MacOS_Build.html), you will find detailed information how to build and compile SUMO in MacOS.

A SUMO model is alread added in the *src/ndnsim/sumo/intersection* folder which can be used for our simulation. You can run the sumo simulation in isolation by using the following command: *sumo-gui -c /your/path/intersection.sumocfg*

To run basic_scenario.py , simply type the following command:
**./waf --pyrun=src/ndnSIM/examples/basic_scenario.py**


