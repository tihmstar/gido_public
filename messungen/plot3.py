import struct
import matplotlib.pyplot as plt
import numpy as np
import sys
import binascii
import tarfile

fontsize = 40
ticksize = 4

SAMPLES_PER_SEC = (40*10**9)
CORRETION_FACTOR = 10**6


if len(sys.argv) < 2:
    print("Usage: %s <path>"%sys.argv[0])
    exit(0)

inpath = sys.argv[1]
prepredir = inpath.split("/")[-1]
try:
    prepredir = inpath.split("/")[-3]
except:
    pass
print("inpath=%s"%inpath)


traces = []
traces2 = []

if inpath[-len(".tar.gz"):] == ".tar.gz":
    print("opening compressed file!")
    f = tarfile.open(inpath,"r")
    mem = f.getmembers()[0]
    f.read = f.extractfile(mem).read
else:
    f = open(inpath,"rb")

r = f.read()
f.close()
traces_per_file = struct.unpack("<I",r[0:4])[0]
print("traces_per_file=%s"%traces_per_file)
point_per_trace = int(((len(r)-4)/traces_per_file)-(4 + 128*2))
point_per_trace_tell = struct.unpack("<I",r[4:8])[0]
print("point_per_trace=%s"%point_per_trace)

if point_per_trace_tell != point_per_trace:
    print("\npoint_per_trace_tell=%s"%point_per_trace_tell)
    print("file is corrupt!")
    exit(1)

for i in range(traces_per_file):
    ms = r[4+i*(4+point_per_trace+128*2):4+(i+1)*(4+point_per_trace+128*2)]
    nop = struct.unpack("<I",ms[0:4])[0]
    ptext = ms[4:4+128]
    trace = ms[4+128*2:]
    t = np.array(bytearray(trace)).astype('int8')
    doAppend = False
#    for j in range(5000,6000):
#        if (t[j]<-50):
#            doAppend = True
#            break
    if doAppend:
        traces2.append(t)
    else:
        traces.append(t)

ax = plt.axes()
ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

plt.yticks([]) #remove yaxis


#f = plt.figure(1)
#addval = 0.03
#plt.subplots_adjust(left=f.subplotpars.left+addval, right=f.subplotpars.right+addval)


print("traces=%d"%len(traces))
print("traces2=%d"%len(traces2))

print("plotting")

SHOWME = None #1

MAXTRACES = 500
#MAXTRACES = 10

tt = traces[0]
plt.axis([(0/SAMPLES_PER_SEC)*CORRETION_FACTOR,((len(tt)-1)/SAMPLES_PER_SEC)*CORRETION_FACTOR,-128,127])

#plot
i=-1
plt.figure(1)
for t in traces:
    tarr = [(x/SAMPLES_PER_SEC)*CORRETION_FACTOR for x in range(len(t))]
    i+=1
    if SHOWME != None:
        if i < SHOWME:
            continue
        if i == SHOWME+1:
            break
    else:
        if i == MAXTRACES:
            break
#    plt.plot(t,linewidth=.5, marker='.')
    plt.plot(tarr,t,linewidth=.5)

#i=-1
#plt.figure(2)
#for t in traces2:
#    i+=1
#    if SHOWME != None:
#        if i < SHOWME:
#            continue
#        if i == SHOWME+1:
#            break
#    else:
#        if i == MAXTRACES:
#            break
##    plt.plot(t,linewidth=.5, marker='.')
#    plt.plot(t,linewidth=.5)

#### BEGIN DEBUG
##step1
#brr = [None] * len(traces[0])
#for i in range(1000,6800):
#    brr[i] = -75
#plt.plot(brr,linewidth=4,color='r')
#brr = [None] * len(traces[0])
#for i in range(6000,11000):
#    brr[i] = 120
#plt.plot(brr,linewidth=4,color='g')

##step 2
#plt.axvline(5700, -128, 127,linewidth=2,color='r')
#plt.axvline(5700+2000, -128, 127,linewidth=2,color='r')
#
#plt.axvline(5700-500, -128, 127,linewidth=2,color='b')
#plt.axvline(5700+2000+500, -128, 127,linewidth=2,color='b')


##step 3
#brr = [None] * len(traces[0])
#for i in range(11000,13000):
#    brr[i] = 125
#plt.plot(brr,linewidth=2,color='r')
#brr = [None] * len(traces[0])
#for i in range(10000,15000):
#    brr[i] = -85
#plt.plot(brr,linewidth=2,color='r')


##step 4
#plt.axvline(10500, -128, 127,linewidth=2,color='r')
#plt.axvline(10500+3000, -128, 127,linewidth=2,color='r')
#
#plt.axvline(10500-500, -128, 127,linewidth=2,color='b')
#plt.axvline(10500+3000+500, -128, 127,linewidth=2,color='b')

##step 5
#brr = [None] * len(traces[0])
#for i in range(0,6700):
#    brr[i] = -55
#plt.plot(brr,linewidth=2,color='r')


#### END DEBUG

plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)
plt.xlabel('time (µs)', fontsize=fontsize)
plt.ylabel('EM-Signal', fontsize=fontsize)

#plt.title(prepredir.replace("_","-"), fontsize=fontsize)

plt.show()
