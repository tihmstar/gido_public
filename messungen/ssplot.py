import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os
import tarfile


fontsize = 30
ticksize = 3


if len(sys.argv) < 4:
    print("Usage: %s <path> <bytenum> <tracefile>" %(sys.argv[0]))

path = sys.argv[1]

if path[-1] != "/":
    path += "/"

input = ["INPUT","OUTPUT"]
byte = 0

byte = int(sys.argv[2])

tracefile = sys.argv[3]


fig, ax1 = plt.subplots()

ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis

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

color = 'grey'
ax1.set_ylabel('EM-Signal (Volt)', fontsize=fontsize)
ax1.set_xlabel('Samples', fontsize=fontsize)
ax1.tick_params(axis='y', labelsize=fontsize)

ax1.plot(t,linewidth=.5, color=color)


ax2.set_ylabel('SNR', fontsize=fontsize)
ax2.tick_params(axis='y', labelsize=fontsize)

ax2.axis([0,len(t),0,1])

ax = plt.gca()
for i in range(8):
    color = next(ax._get_lines.prop_cycler)['color']
    if i==7:
        color = next(ax._get_lines.prop_cycler)['color']

    lastcolor = None
    for ip in input:
        fpath = path+"SNR-"+str(i)+"_"+str(byte)+"_traceSelector_"+ip+"_HW"
        print("read file=%s"%fpath)
        arr = np.memmap(fpath, dtype='float64', mode='r')
        
        label = "block "+str(i+1)
        if lastcolor == color:
            ax2.plot(arr, color=color)
        else:
            ax2.plot(arr, label=label, color=color)
        lastcolor = color


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


#plt.legend(loc="upper right", fontsize=fontsize)
#plt.title("BYTE_"+str(byte)+"_HW", fontsize=fontsize)

plt.xlabel('Samples', fontsize=fontsize)
plt.ylabel('SNR', fontsize=fontsize)


plt.show()
