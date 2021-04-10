import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

POINTS_END = None
#POINTS_END = 1500

SAMPLES_PER_SEC = (40*10**9)
CORRETION_FACTOR = 10**6

fontsize = 40
ticksize = 4

plotpath = None

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))


file = sys.argv[1]
filename = file.split("/")[-1]

arr = np.memmap(file, dtype='float128', mode='r')

xarr = [(x/SAMPLES_PER_SEC)*CORRETION_FACTOR for x in range(len(arr))]

if POINTS_END:
    plt.plot(xarr,arr[0:POINTS_END])
else:
    plt.plot(xarr,arr,color='k')
    
#plt.plot([4.5]*len(arr))
#plt.plot([-4.5]*len(arr))

maxpos = 0
maxval = 0

for i in range(len(arr)):
    if abs(arr[i]) > maxval:
        maxval = abs(arr[i])
        maxpos = i

print("Maxpos=%3d maxval=%f file=%s"%(maxpos,maxval,filename))

xvals = [0,xarr[-1]]

plt.plot(xvals,[4.5,4.5], color='r')
plt.plot(xvals,[-4.5,-4.5], color='r')

ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

plt.axis([0,1.80,-14000,15000])



f = plt.figure(1)
addval = 0.05
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)

plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)
plt.xlabel('time (µs)', fontsize=fontsize)
plt.ylabel('t-value', fontsize=fontsize)
#plt.title(filename, fontsize=fontsize)
plt.show()
