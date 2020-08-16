## Multiple Targets Generation
To find the multiple point of interests/target, you can use the modified_target.py scenario.
The scenario contains a getTargets() module which will give you a list of multiple targets.
You can also configure how many targets you want (e.g., 3, 5, 6 etc.)

To run the scneario: "python3 src/ndnSIM/scenarios/modified_target.py --duration=60 (or whatever) --numberOfInterest=3(default)"

Currently, it sends multiple Interest for this multiple targets.


## Finding numbers of adjusted, collided, passed Cars

To find the number of adjusted, collided, passed car with some other compunf numbers (e.g, adjusted_and_not_collided etc.),
scenario **count_speed_adjustment.py** need to be run.

To run the scenario, type the following command: _python3 src/ndnSIM/scenarios/count_speed_adjustment.py --duration=180 (how long you want to simulate)_

This program will create a csv file named "numbers.csv" in "results" folded with some numbers of every 10 seconds.
