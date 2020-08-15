To find the multiple point of interests/target, you can use the modified_target.py scenario.
The scenario contains a getTargets() module which will give you a list of multiple targets.
You can also configure how many targets you want (e.g., 3, 5, 6 etc.)

To run the scneario: "python3 src/ndnSIM/scenarios/modified_target.py --duration=60 (or whatever) --numberOfInterest=3(default)"

Currently, it sends multiple Interest for this multiple targets.
