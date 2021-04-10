import lecroyparser
import matplotlib.pyplot as plt
import sys
from matplotlib.ticker import ScalarFormatter, FormatStrFormatter
import numpy as np

fontsize = 30
ticksize = 3

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



ax = plt.axes()
ax.xaxis.set_major_formatter(FixedOrderFormatter(-9))

ax.xaxis.offsetText.set_fontsize(fontsize)
ax.xaxis.set_tick_params(width=ticksize,length=ticksize*2)
ax.yaxis.offsetText.set_fontsize(fontsize)
ax.yaxis.set_tick_params(width=ticksize,length=ticksize*2)


plt.axis([0,3500e-9,-0.10,0.08])
plt.xticks([i*1e-9 for i in range(0,3600,500)],fontsize=fontsize)
plt.yticks([i*0.01 for i in range(-10,8,2)],fontsize=fontsize)



if len(sys.argv) < 2:
    print("Usage: %s <path>",sys.argv[0])


prefix = sys.argv[1]
if prefix[-1] != "/":
    prefix += "/"

for i in range(15,-1,-1):
    numstr = str(i)
    while len(numstr) < 5:
        numstr = "0"+numstr
    curpath = prefix + "C2--Trace--"+numstr+".trc"
    print(curpath)
    data = lecroyparser.ScopeData(curpath)
    plt.plot(data.x,data.y)

plt.xlabel('Time in s', fontsize=fontsize)
plt.ylabel('EM-Signal (Volt)', fontsize=fontsize)


title = "Multiple block decryptions"

plt.title(title, fontsize=fontsize)
plt.show()
