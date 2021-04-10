import struct
import matplotlib.pyplot as plt
import numpy as np
import sys

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
print("point_per_trace=%s"%point_per_trace)

for i in range(traces_per_file):
    ms = r[4+i*(4+point_per_trace+16):4+(i+1)*(4+point_per_trace+16)]
    nop = struct.unpack("<I",ms[0:4])[0]
    trace = ms[4:-16]
    ptext = ms[-16:]
    t = np.array(bytearray(trace)).astype('int8')
    traces.append(t)
    

print("plotting")
#plot
i=-1
plt.figure(1)
for t in traces:
    i+=1
    if i == 1000:
        break
    plt.plot(t,linewidth=.5, marker='.')
#    if abs(t[81]) < 5:
#        print("badi=",i)
    
#plt.figure(2)
#plt.plot(traces[1][0:-2],linewidth=.5, marker='.')
#plt.plot(traces[0][2:],linewidth=.5, marker='.')



plt.title("test")
plt.show()
