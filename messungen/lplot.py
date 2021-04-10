import lecroyparser
import matplotlib.pyplot as plt
import sys
from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
import numpy as np

fontsize = 30
ticksize = 3

def min(a,b):
    return a if a < b else b

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


if len(sys.argv) < 2:
    print("Usage: %s <filename>",sys.argv[0])

path = sys.argv[1]


c2path = path.replace("C3--Trace","C2--Trace")
c3path = path.replace("C2--Trace","C3--Trace")
print(c2path)
print(c3path)

c2 = lecroyparser.ScopeData(c2path)
c3 = lecroyparser.ScopeData(c3path)


fig, ax1 = plt.subplots()

color = 'tab:red'
ax1.set_xlabel('Time in s', fontsize=fontsize)
ax1.set_ylabel('EM-Signal (Volt)', color=color, fontsize=fontsize)
ax1.plot(c2.x, c2.y, color=color)
ax1.tick_params(axis='y', labelcolor=color)

ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis

color = 'tab:blue'
ax2.set_ylabel('Volt', color=color, fontsize=fontsize)  # we already handled the x-label with ax1
ax2.plot(c3.x, c3.y, color=color)
ax2.tick_params(axis='y', labelcolor=color)


ax1.xaxis.set_major_formatter(FixedOrderFormatter(-9))
ax1.xaxis.offsetText.set_fontsize(fontsize)

ax1.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax1.yaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax2.yaxis.set_tick_params(width=ticksize,length=ticksize*2)

fig.tight_layout()  # otherwise the right y-label is slightly clipped


xrange = [c2.x[0],3500e-9]

ax2.set_yticks([i*0.1 for i in range(-25,25,5)]) # Volt
ax2.axis([xrange[0],xrange[-1],-3.0,2.4])


ax1.set_yticks([i*0.01 for i in range(-8,8,2)]) # EM
ax1.axis([xrange[0],xrange[-1],-0.10,0.08])

ax1.tick_params(axis='both', which='major', labelsize=fontsize)
ax2.tick_params(axis='both', which='major', labelsize=fontsize)


ax1.set_xticks([i*1e-9 for i in range(0,4500,500)]) # time



blockstr = path.split(".")[0].split("-")[-1]
blockcnt = int(blockstr)+1
title = "Decrypted blocks: %d" % blockcnt



### Analysis ###

    

gpioUP = 0
trigger = 1

for i in range(len(c3.y)):
    if c3.y[i] >= 1:
        gpioUP = i
        break

gpioDOWN = 0

for i in range(gpioUP+100,len(c3.y)):
    if c3.y[i] <= trigger:
        gpioDOWN = i
        break
    
gpiolen = c3.x[gpioDOWN] - c3.x[gpioUP]
print("[%d] GPIO len="%blockcnt,gpiolen)



negpeakcnt = 0
lastaction = 0
lastval = 0
peaks = []
for i in range(len(c2.y)):
    if c2.y[i] > lastval: #we are rising
        if lastaction == -1: #if we were falling before
                peaks.append(i-1)
        lastaction = 1 # we are rising
    elif c2.y[i] < lastval: #we are falling
        if lastaction == 1: #if we were rising before
            peaks.append(i-1)
        lastaction = -1 #we are falling
    lastval = c2.y[i]
    
interesing_peaks = [p for p in peaks if c2.y[p] < -0.025 and c2.y[p] > -0.04 and c2.x[p] > 900e-9 and c2.x[p] < 3500e-9]

while interesing_peaks[-1] - interesing_peaks[-2] < 1500:
    del interesing_peaks[-1]

#print(interesing_peaks)


xpos = [interesing_peaks[0],interesing_peaks[-1]]
yline = min(c2.y[xpos[0]],c2.y[xpos[1]])


peaklen = c2.x[xpos[1]] - c2.x[xpos[0]]
print("[%d] PEAK len="%blockcnt,peaklen)



### editing ###
#ax1.scatter([c2.x[p] for p in interesing_peaks],[c2.y[p] for p in interesing_peaks]) # signal interesting peaks
ax1.plot([c2.x[xpos[0]], c2.x[xpos[1]]],[yline,yline], color='g') ## signal start-stop
ax2.plot([c3.x[gpioUP], c3.x[gpioDOWN]],[1,1], color='g') #gpio over 1V duration





plt.title(title, fontsize=fontsize)
plt.show()
