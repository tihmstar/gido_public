import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

POINTS_PER_TRACE = 120

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

file = sys.argv[1]
filename = file.split("/")[-1]

arr = np.memmap(file, dtype='float64', mode='r')


plt.plot(arr[-POINTS_PER_TRACE:])


plt.title(filename)
#plt.savefig("traces_"+filename.replace(".dat",".png"))
plt.show()
