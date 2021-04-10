import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os
import tarfile

fontsize = 30
ticksize = 3

legendsize = 20

colors = "rgbcmykb"
ci = 0
lastci = -1

if len(sys.argv) < 3:
    print("Usage: %s <tracefile> <path ....>" %(sys.argv[0]))

fig, ax1 = plt.subplots()

ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis

tracefile = sys.argv[1]

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



ax2.set_ylabel('Correlation', fontsize=fontsize)
ax2.tick_params(axis='y', labelsize=fontsize)

ax2.axis([0,len(t),-0.001,0.006])


for p in sys.argv[2:]:
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

    r = ""
    for s in filename.split("_"):
        if "ROUND" in s:
            r = s
            break
    if lastci != ci:
        ax2.plot(arr, label=lastdir, color=colors[ci])
        lastci = ci
    else:
        plt.plot(arr, color=colors[ci])
        
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


#plt.xlabel('Samples', fontsize=fontsize)
#plt.ylabel('Correlation', fontsize=fontsize)

f = plt.figure(1)
#addval = -0.0235
addval = -0.025
#addval = -0.021
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)


leg = plt.legend(loc="upper right", fontsize=legendsize)

for legobj in leg.legendHandles:
    legobj.set_linewidth(ticksize)


#plt.title(lastdir, fontsize=fontsize)
#plt.savefig("traces_"+filename.replace(".dat",".png"))
plt.show()
