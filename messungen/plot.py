import struct
import matplotlib.pyplot as plt
import numpy as np
import sys
import binascii

if len(sys.argv) < 2:
    print("Usage: %s <path>"%sys.argv[0])
    exit(0)
    
inpath = sys.argv[1]
print("inpath=%s"%inpath)


traces = []

f = open(inpath,"rb")
r = f.read()
f.close()
traces_per_file = struct.unpack("<I",r[0:4])[0]

print("traces_per_file=%s"%traces_per_file)
point_per_trace = int(((len(r)-4)/traces_per_file)-20)
point_per_trace_tell = struct.unpack("<I",r[4:8])[0]
print("point_per_trace=%s"%point_per_trace)

if point_per_trace_tell != point_per_trace:
    print("\npoint_per_trace_tell=%s"%point_per_trace_tell)
    print("file is corrupt!")
    exit(1)

for i in range(traces_per_file):
    ms = r[4+i*(4+point_per_trace+16):4+(i+1)*(4+point_per_trace+16)]
    nop = struct.unpack("<I",ms[0:4])[0]
    ptext = ms[4:4+16]
    trace = ms[4+16:]
    t = np.array(bytearray(trace)).astype('int8')    
    traces.append(t)
    

print("plotting")

SHOWME = None #950

MAXTRACES = 500
#plot
i=-1
plt.figure(1)
for t in traces:
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
    plt.plot(t,linewidth=.5)
#    if abs(t[1000]) < 10:
#        print("badi=",i)
    

#i=-1
#plt.figure(2)
#for t in traces:
#    i+=1
#    if i == MAXTRACES:
#        break
#    if abs(t[1000]) < 10:
##        plt.plot(t,linewidth=.5, marker='.')
#        plt.plot(t,linewidth=.5)
#    else:
#        i-=1

plt.title("test")
plt.show()
