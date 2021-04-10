import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os
import tarfile

fontsize = 30
ticksize = 3


if len(sys.argv) < 3:
    print("Usage: %s <path> <tracefile>" %(sys.argv[0]))


tracefile = sys.argv[2]


file = sys.argv[1]
filename = file.split("/")[-1]

arr = np.memmap(file, dtype='float128', mode='r')

fig, ax2 = plt.subplots()


ax1 = ax2.twinx()  # instantiate a second axes that shares the same x-axis

color = 'tab:red'
ax2.set_ylabel('EM-Signal (Volt)', color=color, fontsize=fontsize)
ax2.set_xlabel('Samples', fontsize=fontsize)
ax2.tick_params(axis='y', labelcolor=color, labelsize=fontsize)

if tracefile[-len(".tar.gz"):] == ".tar.gz":
    print("opening compressed file!")
    f = tarfile.open(tracefile,"r")
    mem = f.getmembers()[0]
    f.read = f.extractfile(mem).read
else:
    f = open(tracefile,"rb")

r = f.read()
f.close()
traces_per_file = struct.unpack("<I",r[0:4])[0]
print("traces_per_file=%s"%traces_per_file)
point_per_trace = int(((len(r)-4)/traces_per_file)-(4 + 128*2))
point_per_trace_tell = struct.unpack("<I",r[4:8])[0]
print("point_per_trace=%s"%point_per_trace)

i=0
ms = r[4+i*(4+point_per_trace+128*2):4+(i+1)*(4+point_per_trace+128*2)]
nop = struct.unpack("<I",ms[0:4])[0]
ptext = ms[4:4+128]
trace = ms[4+128*2:]
t = np.array(bytearray(trace)).astype('int8')
ax2.plot(t,linewidth=.5, color=color)




color = 'tab:blue'
ax1.set_ylabel('t-value', color=color, fontsize=fontsize)
ax1.plot(arr, color=color)
ax1.tick_params(axis='y', labelcolor=color, labelsize=fontsize)

ax1.tick_params(axis='both', which='major', labelsize=fontsize)
ax2.tick_params(axis='both', which='major', labelsize=fontsize)


ax1.xaxis.offsetText.set_fontsize(fontsize)
ax1.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax1.yaxis.offsetText.set_fontsize(fontsize)
ax1.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

ax2.xaxis.offsetText.set_fontsize(fontsize)
ax2.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax2.yaxis.offsetText.set_fontsize(fontsize)
ax2.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

f = plt.figure(1)
addval = -0.035
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)



#plt.plot([4.5]*len(arr))
#plt.plot([-4.5]*len(arr))

maxpos = 0
maxval = 0

for i in range(len(arr)):
    if abs(arr[i]) > maxval:
        maxval = abs(arr[i])
        maxpos = i

print("Maxpos=%3d maxval=%f file=%s"%(maxpos,maxval,filename))



#plt.xticks(fontsize=fontsize)
#plt.yticks(fontsize=fontsize)
#plt.title(filename, fontsize=fontsize)
plt.show()
