import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os


fontsize = 30
ticksize = 3

legendsize = 20


if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

path = sys.argv[1]

if path[-1] != "/":
    path += "/"

input = ["INPUT","OUTPUT"]
byte = 0

if len(sys.argv) >=3:
    byte = int(sys.argv[2])


parr = []
ax = plt.gca()
for i in range(8):
    color = next(ax._get_lines.prop_cycler)['color']
    lastcolor = None
    for ip in input:
        fpath = path+"SNR-"+str(i)+"_"+str(byte)+"_traceSelector_"+ip+"_HW"
        print("read file=%s"%fpath)
        arr = np.memmap(fpath, dtype='float64', mode='r')
        
        label = "block "+str(i+1)
        if lastcolor == color:
            plt.plot(arr, color=color)
        else:
            plt.plot(arr, label=label, color=color)
        lastcolor = color


plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)

ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

leg = plt.legend(loc="upper right", fontsize=legendsize)

for legobj in leg.legendHandles:
    legobj.set_linewidth(ticksize)

#plt.title("BYTE_"+str(byte)+"_HW", fontsize=fontsize)

plt.xlabel('Samples', fontsize=fontsize)
plt.ylabel('SNR', fontsize=fontsize)


plt.show()
