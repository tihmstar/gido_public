import struct
import sys
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import mmap
import os
from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
import tarfile

fontsize = 40
ticksize = 4

#fontsize = 10
#ticksize = 1

TRACES_CORRRECTION = 1e-6
CORRELATION_CORRECTION = 1e3


class FixedOrderFormatter(ScalarFormatter):
    """Formats axis ticks using scientific notation with a constant order of
    magnitude"""
    def __init__(self, order_of_mag=0, useOffset=True, useMathText=False):
        self._order_of_mag = order_of_mag
        ScalarFormatter.__init__(self, useOffset=useOffset,
                                 useMathText=useMathText)
    def _set_orderOfMagnitude(self, range):
        """Over-riding this to avoid having orderOfMagnitude reset elsewhere"""
        self.orderOfMagnitude = self._order_of_mag

POS = None
#POS = 3817 #round 1
#POS = 3616 #round 2

printkey = None

if len(sys.argv) < 2:
    print("Usage: %s <filename>" %(sys.argv[0]))

if len(sys.argv) >= 3:
    POS = int(sys.argv[2])
    print("set POS to "+str(POS))
        
if len(sys.argv) >= 4:
    printkey = int(sys.argv[3])
    print("set printkey to "+str(printkey))


path = sys.argv[1]

filename = path.split("/")[-1]
path = "/".join(path.split("/")[0:-1]) + "/"
if path == "/":
    path = "./"

dirs = [int(d) for d in os.listdir(path) if os.path.isdir(path+"/"+d)]
dirs = [str(i) for i in dirs]

tars = [t for t in os.listdir(path) if t[-7:] == ".tar.gz"]
for t in tars:
    if t[0:-7] in dirs:
        dirs.remove(t[0:-7])
    dirs.append(t)
dirs.append(".")
print(dirs)

vals = []

for i in range(0x100):
    vals.append({})

cnt = 0
allcnt = len(dirs)
tar = None

if POS == None:
    print("Error: POS not set!")
    exit(1)

for d in dirs:
    cnt+=1
    dpath = path + d
    if tar:
        tar.close()
        tar = None
        
    if d[-7:] == ".tar.gz":
        tar = tarfile.open(dpath,"r")
        dpath = d[0:-7]
        print("[%02d of %02d] reading (compressed) %s"%(cnt,allcnt,d))
    else:
        print("[%02d of %02d] reading %s"%(cnt,allcnt,d))
    npath = dpath + "/" + "numTraces.txt"
    x = 0
    try:
        if tar:
            fm = tar.getmember(npath)
            em = tar.extractfile(fm)
            r = em.read()
            x = int(r)
        else:
            with open(npath) as f:
                r = f.read()
                x = int(r)
    except:
        continue
    curi=0
    try:
        for i in range(0x100):
            curi = i
            cpath = dpath + "/" + filename + str(i)
            y = 0
            data = None
            if tar:
                fm = tar.getmember(cpath)
                em = tar.extractfile(fm)
                data = em.read()
                arr = np.frombuffer(bytearray(data), dtype='float64')
            else:
                arr = np.memmap(cpath, dtype='float64', mode='r')
            y = arr[POS]
            vals[i][x] = y
    except:
        print("failed to load i=%d"%curi)
        if curi != 0:
            exit(1)
        continue
if tar:
    tar.close()
    tar = None


#plt.ylabel('Correlation')
#plt.xlabel('Time')


xvals = []
yvals = []

for i in range(0x100):
    lists = sorted(vals[i].items()) # sorted by key, return a list of tuples
    x, y = zip(*lists) # unpack a list of pairs into two tuples
    xvals.append(x)
    yvals.append(y)

#print("vals")
#print(vals)
#print("xvals")
#print(xvals[153])
#print("yvals")
#print(yvals[153])

print("len xvals[0]=%d"%len(xvals[0]))
print("len yvals[0]=%d"%len(yvals[0]))

pv = 0
for v in xvals[0]:
    if pv > v:
        print("ERROR x sort pv=%d v=%d"%(pv,v))
    pv = v


lmaxkey = -1
lmaxval = None
for i in range(0x100):
    if lmaxval == None or yvals[i][-1] > lmaxval:
        lmaxval = yvals[i][-1]
        lmaxkey = i

print("lmaxkey=%d"%lmaxkey)
print("lmaxval=%f"%lmaxval)

if printkey != None:
    lmaxkey = printkey
    print("custom lmaxkey=%d"%lmaxkey)


ax = plt.axes()
ax.xaxis.set_major_formatter(FixedOrderFormatter(6))


ax.yaxis.set_major_formatter(FixedOrderFormatter(-3))


numberOfTraces = int(420e6*TRACES_CORRRECTION)

#plt.axis([0,numberOfTraces,-0.0003*CORRELATION_CORRECTION,0.0003*CORRELATION_CORRECTION])
plt.axis([0,numberOfTraces,-0.0002*CORRELATION_CORRECTION,0.0004*CORRELATION_CORRECTION])


plt.xticks(range(0,int(numberOfTraces),int(50e6*TRACES_CORRRECTION)), fontsize=fontsize)
plt.yticks(fontsize=fontsize)


linewidth=4

for i in range(0x100):
    if i == lmaxkey:
        continue
    cxvals = [x*TRACES_CORRRECTION for x in xvals[i]]
    cyvals = [y*CORRELATION_CORRECTION for y in yvals[i]]

    plt.plot(cxvals, cyvals, color='lightgrey',linewidth=linewidth)

mxvals = [x*TRACES_CORRRECTION for x in xvals[lmaxkey]]
myvals = [y*CORRELATION_CORRECTION for y in yvals[lmaxkey]]
plt.plot(mxvals, myvals, color='k',linewidth=linewidth)


ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

f = plt.figure(1)
addval = 0.04
plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)


#plt.xlabel('Number of traces', fontsize=fontsize)
plt.xlabel('Number of traces '+f'(10\N{SUPERSCRIPT SIX})', fontsize=fontsize)
#plt.ylabel('Correlation', fontsize=fontsize)
plt.ylabel('Correlation '+f'(10\N{SUPERSCRIPT MINUS}\N{SUPERSCRIPT THREE})', fontsize=fontsize)


#title = filename + " MAXKEY=" + str(lmaxkey) + " POS="+str(POS)
#title = title.replace("_","-")
#plt.title(title, fontsize=fontsize)


plt.show()
