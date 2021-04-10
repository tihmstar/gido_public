
import os
import shutil
import sys
import copy

selectors = [
'SELECTOR_PLAIN',
'SELECTOR_PLAIN_HW',
'SELECTOR_PLAIN_HW4',
'SELECTOR_PLAIN_KEY_HD',
'SELECTOR_SBOX',
'SELECTOR_SBOX_HW',
'SELECTOR_ZEROVALUE',
'SELECTOR_CIPHER',
'SELECTOR_CIPHER_HW',
'SELECTOR_CIPHER_HW4',
'SELECTOR_TTABLET0_HW4',
'SELECTOR_TTABLET1_HW4',
'SELECTOR_TTABLET2_HW4',
'SELECTOR_TTABLET3_HW4',
'SELECTOR_TTABLET0T1_HD4',
'SELECTOR_TTABLET0T1T2_HD4',
'SELECTOR_TTABLET0T1T2T3_HD4',
'SELECTOR_TTABLET0T1T2T3K1_HD4',
'SELECTOR_TTABLET0P0_HD4',
'SELECTOR_TTABLET0P0K0_HD4',
'SELECTOR_P0K0_HD4',
'SELECTOR_ROUND_HW',
'SELECTOR_ROUND_HW4',
'SELECTOR_ROUND_VALUE'
]

if len(sys.argv) < 2:
    print("Usage: %s <path to traces> {selector}"%sys.argv[0])
    exit(1)

tracespath= sys.argv[1]

if len(sys.argv) >=3:
    sel = sys.argv[2]
    print("running sel: %s"%sel)
    selectors = [sel]


f = open("kernels.cl","r")
r = f.read()
f.close()

selector_value = "8"
selector_round = "4"

try:
    os.mkdir("results_runner")
except:
    pass
for sel in selectors:
    cursel = sel+"_"+selector_value
    dstdir = "results_runner/"+cursel
    dstfile = "SNR-"+cursel
    
    print("checking="+dstdir+"/"+dstfile)
    if os.path.isfile(dstdir+"/"+dstfile):
        print("skipping alread existing file=%s"%dstfile)
        continue
        
    try:
        os.mkdir(dstdir)
    except:
        pass
    try:
        os.unlink(dstdir+"/snrtest_opencl")
    except:
        pass
    shutil.copyfile("snrtest_opencl",dstdir+"/snrtest_opencl")
    try:
        os.chmod(dstdir+"/snrtest_opencl", 600)
    except:
        pass
    os.chdir(dstdir)
    
    curtrpath = tracespath
    if curtrpath[0] != '/':
        curtrpath = "../../"+tracespath
    cmd = "./snrtest_opencl "+curtrpath+ " "+ dstfile
    
    newkernel = open("kernels.cl","w")
    kernncopy = copy.deepcopy(r)
    kernncopy = kernncopy.replace("#define G_SELECTOR_TYPE","#define G_SELECTOR_TYPE "+sel)
    kernncopy = kernncopy.replace("#define G_SELECTOR_VALUE","#define G_SELECTOR_VALUE "+selector_value)
    kernncopy = kernncopy.replace("#define G_SELECTOR_ROUND","#define G_SELECTOR_ROUND "+selector_round)
    newkernel.write(kernncopy)
    newkernel.close()
    print("running: "+cmd)
    os.system(cmd)
    os.chdir("../../")
