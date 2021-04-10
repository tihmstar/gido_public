import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

POINTS_END = None
fontsize = 30
ticksize = 3

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

file = sys.argv[1]
filename = file.split("/")[-1]
path = "/".join(file.split("/")[0:-1])


arr = np.memmap(file, dtype='float64', mode='r')
#arr = np.memmap(file, dtype='float32', mode='r')

if POINTS_END:
    plt.plot(arr[0:POINTS_END])
else:
    plt.plot(arr)
    
#plt.plot([4.5]*len(arr))
#plt.plot([-4.5]*len(arr))

ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)

f = plt.figure(1)
addval = 0.05
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)


down = int(plt.ylim()[0]) - 1
up = int(plt.ylim()[1]) + 1

c = "rbgcykwm"
cc = 0

#fpos = open(path+"/pos.txt","r")
#for l in fpos.read().split("\n"):
#    if not len(l):
#        continue
#    begin = int(l.split("_")[0])
#    end = int(l.split("_")[1])
#    plt.vlines([begin, end], ymin = down, ymax = up, color=c[cc])
#    cc = (cc+1)%len(c)

#plt.vlines([28863, 31863], ymin = down, ymax = up, color=c[cc])

try:
    with open(path+"/peak.txt","r") as f:
        for l in f.read().split("\n"):
            if not len(l):
                continue
            x = int(l)
            plt.scatter(x, arr[x])
except:
    pass
try:
    with open(path+"/means.txt","r") as f:
        for l in f.read().split("\n"):
            if not len(l):
                continue
            x = float(l)
            plt.plot([x]*len(arr))
except:
    pass


try:
    with open(path+"/vlines.txt","r") as f:
        for l in f.read().split("\n"):
            if not len(l):
                continue
            begin = int(l.split("_")[0])
            end = int(l.split("_")[1])
            plt.vlines([begin, end], ymin = down, ymax = up, color=c[cc])
except:
    pass



maxpos = 0
maxval = 0

for i in range(len(arr)):
    if abs(arr[i]) > maxval:
        maxval = abs(arr[i])
        maxpos = i

print("Maxpos=%3d maxval=%f file=%s"%(maxpos,maxval,filename))


plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)
plt.xlabel('Samples', fontsize=fontsize)
plt.ylabel('EM-Signal (Volt)', fontsize=fontsize)
plt.title(filename.replace("_","-"), fontsize=fontsize)

plt.show()
