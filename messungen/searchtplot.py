import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

POINTS_END = None
#POINTS_END = 18410

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

file = sys.argv[1]
filename = file.split("/")[-1]

arr = np.memmap(file, dtype='float64', mode='r')

if POINTS_END:
    plt.plot(arr[0:POINTS_END])
else:
    plt.plot(arr)
    
#plt.plot([4.5]*len(arr))
#plt.plot([-4.5]*len(arr))

maxpos = 0
maxval = 0
avgval = 0.0
avgcnt = 0.0

for i in range(len(arr)):
    if abs(arr[i]) > maxval:
        maxval = abs(arr[i])
        maxpos = i
    avgcnt += 1
    avgval += (arr[i]-avgval)/avgcnt
    

print("Maxpos=%3d maxval=%f avgval=%f file=%s"%(maxpos,maxval,avgval,filename))

if maxval < avgval * 25:
    print("maxval too low to show")
    exit(1)

plt.title(filename)
print("saving="+file + ".png")
plt.savefig(file + ".png")
#plt.show()
