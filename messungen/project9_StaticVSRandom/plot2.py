import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

#maxval = 802
#maxval = 802
maxval = 75

TRACES_IN_FILE = 1000000
TRACES_STEPS = TRACES_IN_FILE
POINTS_PER_TRACE = 120


plotsInFile = int(TRACES_IN_FILE/TRACES_STEPS)

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

file = sys.argv[1]
filename = file.split("/")[-1]

arr = np.memmap(file, dtype='float64', mode='r')

parr = []

for i in range(plotsInFile):
    parr.append(arr[i*POINTS_PER_TRACE+maxval])

plt.plot([i*TRACES_STEPS for i in range(1,plotsInFile+1)],parr)


plt.title(filename)
#plt.savefig("traces_"+filename.replace(".dat",".png"))
plt.show()
