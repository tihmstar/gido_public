import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

fontsize = 30
ticksize = 3

legendsize = 18

POINTS_END = None

if len(sys.argv) < 2:
    print("Usage: %s <path ....>" %(sys.argv[0]))

for p in sys.argv[1:]:
    file = p
    filename = file.split("/")[-1]
    try:
        lastdir = file.split("/")[-2]
    except:
        lastdir = filename
    try:
        prepredir = file.split("/")[-3]
    except:
        prepredir = filename

    
    arr = np.memmap(file, dtype='float64', mode='r')

    r = "byte_"+filename.split("_")[-1]
#    if POINTS_END:
#        plt.plot(arr[0:POINTS_END], label=r)
#    else:
##        plt.plot(arr, label=r)
#        plt.plot(arr, linestyle="", markersize=1, marker='.')

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
    aa = [None]*len(arr)
    aa[maxpos] = arr[maxpos]
    plt.plot(aa, linestyle="", markersize=20, marker='.', label=r)


    print("Maxpos=%3d maxval=%f avgval=%f file=%s"%(maxpos,maxval,avgval,filename))

leg = plt.legend(loc="upper right", fontsize=legendsize)

for legobj in leg.legendHandles:
    legobj.set_linewidth(ticksize)

ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

f = plt.figure(1)
addval = 0.04
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)


plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)
plt.xlabel('Samples', fontsize=fontsize)
plt.ylabel('Correlation', fontsize=fontsize)

#plt.title(lastdir.replace("_","-"), fontsize=fontsize)
#plt.savefig("traces_"+filename.replace(".dat",".png"))
plt.show()
