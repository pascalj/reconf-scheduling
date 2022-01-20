#!/usr/bin/env python3

# lud_internal = 234us
# lud_diagonal = 235us
# lud_perimeter = 235us

import json
import random
import sys

tasks = {}
dependencies = {}
configs = ["config1", "config2"]
P_config = ["config1", "config1", "config1", "config2", "config2", "config2", "config2"]

if len(sys.argv) < 2:
    print("Usage: generate_lu.py <nblocks>")
    exit(1)

def cost_fun(idx):
    (it, i, j) = ridx(idx)
    if it == i and it == j:
        return [235, False, False, False, False, False, False]
    if it == j or it == i:
        return [False, 235, False, False, False, False, False]
    else:
        return [False, False, 235, 120, 120, 120, 120]

blocks = int(sys.argv[1])


tasklabels = []


nt = 0

def idx(it, i, j):
    return j + blocks * (i + blocks * it)

def ridx(idx):
    j = int(idx / (blocks * blocks))
    idx -= int((j * blocks * blocks))
    i = int(idx / blocks)
    it = int(idx % blocks)
    return (int(j), int(i), int(it))

def dep(f, t):
    c = dependencies.get(f, [])
    c.append(t)
    dependencies[f] = c

def task(it, i, j):
    global nt
    tasks[idx(it, i, j)] = nt
    tasklabels.append(f"({it},{i},{j})")
    nt += 1

for it in range(blocks):
    # dependencies for all inner blocks
    task(it, it, it)
    if it > 0:
        for i in range(it - 1, blocks):
            for j in range(it - 1, blocks):
                dep(idx(it, it, it), idx(it - 1, i, j))
        
    for j in range(it + 1, blocks):
        task(it, it, j)
        task(it, j, it)
        dep(idx(it, it, j), idx(it, it, it))
        dep(idx(it, j, it), idx(it, it, it))

    for i in range(it + 1, blocks):
        for j in range(it + 1, blocks):
            task(it, i, j)
            dep(idx(it, i, j), idx(it, i, it))
            dep(idx(it, i, j), idx(it, it, j))

ntasks = len(tasks)
deps = [[False for y in range(ntasks)] for x in range(ntasks)]
for f in dependencies:
    for t in dependencies[f]:
        deps[tasks[f]][tasks[t]] = True


cost = [cost_fun(t) for t in tasks]

j = {
        "C": {"set": [{"e" : x} for x in configs]},
        "P_config": P_config,
        "deps": deps,
        "cost": cost,
        "tasklabels": tasklabels,
        "ntasks": ntasks,
        "nprocs": len(P_config)
}

print(json.dumps(j))
