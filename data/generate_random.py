#!/usr/bin/env python3


import json
import random
import sys
from random import seed, randrange

seed(12345)

dependencies = {}
tasklabels = []
configs = ["config1", "config2"]
P_config = ["config1", "config1", "config1", "config2", "config2", "config2", "config2"]

if len(sys.argv) < 2:
    print("Usage: generate_random.py <ntasks> [connectivity] [concurrency]")
    print("")
    print("connectivity: chance of a connection from one node to its predecessor")
    print("concurrency: maxmium concurrent tasks")
    exit(1)

costmax = 500

def cost_fun(idx):
    return [randrange(costmax) for p in P_config]

ntasks = int(sys.argv[1])
connectivity = 10

if (len(sys.argv) > 2):
    connectivity = int(sys.argv[2])
if (len(sys.argv) > 3):
    concurrency = int(sys.argv[3])

tasks = range(ntasks)


deps = [[False for y in range(ntasks)] for x in range(ntasks)]
for f in tasks:
    for t in tasks:
        if  f < t:
            if randrange(100) < connectivity:
                deps[f][t] = True


cost = [cost_fun(t) for t in tasks]

j = {
        "C": {"set": [{"e" : x} for x in configs]},
        "P_config": P_config,
        "deps": deps,
        "cost": cost,
        "tasklabels": [str(t) for t in tasks],
        "ntasks": ntasks,
        "nprocs": len(P_config)
}

print(json.dumps(j))
