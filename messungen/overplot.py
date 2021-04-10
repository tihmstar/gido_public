import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

POINTS_END = None

fontsize = 40
ticksize = 4

SAMPLES_PER_SEC = (40*10**9)
CORRETION_FACTOR = 10**6

legendsize = 20

yminval = None
ymaxval = None


if len(sys.argv) < 2:
    print("Usage: %s <path ....>" %(sys.argv[0]))

colormap = plt.cm.nipy_spectral
#colormap = plt.get_cmap('gist_rainbow')
num_colors = 16
#c = [colormap(i) for i in np.linspace(0, 1,num_colors)]
#colors = [c[0],c[1],c[10],c[3],c[8],c[2],c[15],c[7],c[13],c[5],c[12],c[4],c[6],c[9],c[14]]
colors = [colormap(i) for i in np.linspace(0, 10,10*num_colors)]

colors = ["firebrick","dodgerblue","gold","limegreen","magenta","teal","dimgrey","deeppink","mediumslateblue","orange","red","tomato","forestgreen","darkviolet"]

num = -1
for p in sys.argv[1:]:
    num+=1
    file = p
    filename = file.split("/")[-1]
    try:
        lastdir = file.split("/")[-2]
    except:
        lastdir = "_".join(filename.split("_")[0:-2])
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

    xarr = [(x/SAMPLES_PER_SEC)*CORRETION_FACTOR for x in range(len(arr))]


    r = ""
    for s in filename.split("_"):
        if "ROUND" in s:
            r= "ROUND_" + filename.split("_")[-2].split("-")[0]
            break
    if POINTS_END:
        plt.plot(xarr,arr[0:POINTS_END], label=r)
    else:
        myc = colors[num]
        
        if len(sys.argv[1:]) == 1:
            myc = 'k'
        
        plt.plot(xarr,arr, label=r, color=myc)
#        plt.plot(xarr,arr, label=r)

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


leg = plt.legend(loc="upper right", fontsize=legendsize)
#
#for legobj in leg.legendHandles:
#    legobj.set_linewidth(ticksize)
leg.remove()


#bnumstr =lastdir.split("_")[0].split("-")[1]
#bnumstr = str(int(bnumstr)+1)
#title = "Block "+bnumstr

#title = "".join([s for s in lastdir.split("_") if "HW" in s])

#plt.title(title, fontsize=fontsize)
plt.show()
