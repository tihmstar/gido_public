import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os
from matplotlib.ticker import ScalarFormatter, FormatStrFormatter


fontsize = 40
ticksize = 4

SAMPLES_PER_SEC = (40*10**9)
CORRETION_FACTOR = 10**6


legendsize = 20


colors = "rgbcmykb"
ci = 0
lastci = -1
yminval = None
ymaxval = None


if len(sys.argv) < 2:
    print("Usage: %s <path ....>" %(sys.argv[0]))


for p in sys.argv[1:]:
    if p == ";":
        ci = (ci + 1) % len(colors)
        continue
    file = p
    filename = file.split("/")[-1]
    try:
        lastdir = file.split("/")[-2]
    except:
        bnumstr = filename.split("_")[0].split("-")[1]
        bnumstr = str(int(bnumstr)+1)
        lastdir = "Block "+bnumstr
    try:
        prepredir = file.split("/")[-3]
    except:
        prepredir = filename

    
    arr = np.memmap(file, dtype='float64', mode='r')
    arr = [a*10**(3) for a in arr]

    for a in arr:
        if not yminval or a < yminval:
            yminval = a
        if not ymaxval or a > ymaxval:
            ymaxval = a

    r = ""
    for s in filename.split("_"):
        if "ROUND" in s:
            r = s
            break
    xarr = [(x/SAMPLES_PER_SEC)*CORRETION_FACTOR for x in range(len(arr))]
    
    if lastci != ci:
        plt.plot(xarr,arr, label=lastdir, color=colors[ci])
        lastci = ci
    else:
        plt.plot(xarr,arr, color=colors[ci])
        
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

plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)

plt.axis([(0/SAMPLES_PER_SEC)*CORRETION_FACTOR,((len(arr)-1)/SAMPLES_PER_SEC)*CORRETION_FACTOR,yminval,ymaxval])


plt.xlabel('time (µs)', fontsize=fontsize)
plt.ylabel('Correlation '+f'(10\N{SUPERSCRIPT MINUS}\N{SUPERSCRIPT THREE})', fontsize=fontsize)

ax = plt.axes()

ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)



f = plt.figure(1)
addval = 0.035
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)


leg = plt.legend(loc="lower right", fontsize=legendsize)
#
#for legobj in leg.legendHandles:
#    legobj.set_linewidth(ticksize)
leg.remove()

#plt.title(lastdir, fontsize=fontsize)
#plt.savefig("traces_"+filename.replace(".dat",".png"))
plt.show()
