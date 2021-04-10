import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

file = sys.argv[1]
filename = file.split("/")[-1]

arr = np.memmap(file, dtype='float64', mode='r')
plt.plot(arr)

plt.title(filename)
print("saving="+file + ".png")
plt.savefig(file + ".png")
