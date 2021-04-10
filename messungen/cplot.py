import struct
import sys
import matplotlib.pyplot as plt
import numpy as np
import mmap
import os

fontsize = 40
ticksize = 4
linewidth=4


#fontsize = 10
#ticksize = 1

SAMPLES_PER_SEC = (40*10**9)
CORRETION_FACTOR = 10**6

yminval = None
ymaxval = None


POS = None
#POS = 1639

myrange = None
#test/debug key
#myrange = [3050, 3150] #first round keys // at ~3100
#myrange = [2890, 2910] #second round keys // at ~2900

#GID attack! #first round key at ~7686 block3
#myrange = [7000, 7900]
#myrange = [7600, 7800]
#myrange = [7670, 7700] #use this
#myrange = [7680, 7690]

#GID attack! #second round key at ~ block3
#myrange = [7480, 7490]


#GID attack! #first round key at ~4686 block2
#myrange = [4670, 4690]

#GID attack! #second round key at ~4484 block2
#myrange = [4400, 4500]
#myrange = [4470, 4490] #use this
#myrange = [4484, 4488]

###
##GID256 attack!
###

#GID256 attack! #block 2 - round 1 at ~7248 --verified
#myrange = [7000, 8000]
#myrange = [7200, 7300]
#myrange = [7240, 7260]
#myrange = [7245, 7255]



#GID256 attack! #block 4 - (starting at 11000) - round 1 at ~3816
#myrange = [3800, 3820]


#GID256 attack! #block 4 - (starting at 11000) - round 2 at ~3616
#myrange = [3600, 3620]





def doabs(v):
#    return abs(v)
    return v #don't do abs()

if len(sys.argv) < 2:
    print("Usage: %s <path>" %(sys.argv[0]))

if len(sys.argv) >= 4:
    myrange = [int(sys.argv[2]), int(sys.argv[3])]
    POS = None
    print("set range to [%d, %d]"%(myrange[0],myrange[1]))
elif len(sys.argv) >= 3:
    POS = int(sys.argv[2])
    print("set POS to "+str(POS))


file = sys.argv[1]
filename = file.split("/")[-1]
dirname = filename
try:
    dirname = file.split("/")[-2]
except:
    pass

while True:
    parr = []
    gmaxpos = 0
    gmaxval = 0
    gmaxkey = 0

    lmaxval = 0
    lmaxkey = 0

    for i in range(0x100):
        fn = file + str(i)
        print("processing fn="+fn)
        arr = np.memmap(fn, dtype='float64', mode='r')
        if myrange:
            srange = range(myrange[0],myrange[1])
        else:
            srange = range(len(arr))
        if POS != None:
            parr.append(arr[POS])
        else:
            cmaxval = 0
            cmaxpos = 0
            
            for z in srange:
                if doabs(arr[z]) > cmaxval:
                    cmaxval = doabs(arr[z])
                    cmaxpos = z
            parr.append(arr[cmaxpos])
        for z in srange:
            if doabs(arr[z]) > gmaxval:
                gmaxval = doabs(arr[z])
                gmaxpos = z
                gmaxkey = i
        if doabs(parr[i]) > lmaxval:
            lmaxval = doabs(parr[i])
            lmaxkey = i
    if POS != None:
        break
    print("FOUND maxpos at: gmaxkey=%3d gmaxpos=%3d gmaxval=%f file=%s"%(gmaxkey,gmaxpos,gmaxval,dirname))
    POS = gmaxpos
    myrange = None


xparr = [x for x in range(len(parr))]
parr = [a*10**(3) for a in parr]

for a in parr:
    if not yminval or a < yminval:
        yminval = a
    if not ymaxval or a > ymaxval:
        ymaxval = a

#plt.axis([-1,len(parr),yminval,ymaxval])
madd = (ymaxval - yminval) * 0.013
plt.axis([-1,len(parr),yminval-madd,ymaxval+madd])


plt.plot(xparr,parr, color='k', linewidth=linewidth)

print("gmaxkey=%3d gmaxpos=%3d gmaxval=%f file=%s"%(gmaxkey,gmaxpos,gmaxval,dirname))
print("lmaxkey=%3d lmaxval=%f file=%s"%(lmaxkey,lmaxval,dirname))

#sort by key prob
keylist = []
for i in range(len(parr)):
    keylist.append((i,parr[i]))
def takeSecond(elem):
    return doabs(elem[1])
keylist.sort(key=takeSecond,reverse=True)

for i in range(15):
    print("[%d] KEY=0x%02x prob=%f file=%s" %(i,keylist[i][0],keylist[i][1],file))

#plt.ylabel('Correlation', fontsize=fontsize)
plt.ylabel('Correlation '+f'(10\N{SUPERSCRIPT MINUS}\N{SUPERSCRIPT THREE})', fontsize=fontsize)
plt.xlabel('Key', fontsize=fontsize)

plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)

ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)


f = plt.figure(1)
addval = 0.06
addup = 0.05
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval, bottom=f.subplotpars.bottom+addup, top=f.subplotpars.top+addup)


#title = dirname + " MAXKEY=" + str(lmaxkey) + " POS="+str(POS)
#title = title.replace("_","-")
#plt.title(title, fontsize=fontsize)

#plt.savefig("traces_POS_"+str(POS)+"_"+filename.replace(".dat",".png"))
plt.show()
