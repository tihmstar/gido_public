import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os


#MAX_VALUES = 1800
MAX_VALUES = 80000

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

file = sys.argv[1]
filename = file.split("/")[-1]

parr = []
for i in range(MAX_VALUES):
    parr.append([])

for i in range(1,100000):
    fpath = file+"_"+str(i)
    try:
        arr = np.memmap(fpath, dtype='float64', mode='r')
    except:
        break;
    print("read file=%s"%fpath)
    
    for j in range(MAX_VALUES):
        parr[j].append(arr[j])


for ppp in parr:
    plt.plot([i for i in range(1,len(ppp)+1)],ppp)
    

plt.title(filename)
#plt.savefig("traces_"+filename.replace(".dat",".png"))
plt.show()
