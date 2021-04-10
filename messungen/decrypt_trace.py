import struct
import sys
import binascii
import tarfile
import usb
import recovery

BLOCKS_CNT = 8

if len(sys.argv) < 2:
    print("Usage: %s <path>"%sys.argv[0])
    exit(0)

infile = sys.argv[1]


if infile[-len(".tar.gz"):] == ".tar.gz":
    print("cant open compressed file!")
    exit(1)
else:
    f = open(infile,"rb+")

r = f.read(8)

traces_per_file = struct.unpack("<I",r[0:4])[0]
print("traces_per_file=%s"%traces_per_file)
point_per_trace_tell = struct.unpack("<I",r[4:8])[0]
print("point_per_trace_tell=%s"%point_per_trace_tell)

didRead = 4

f.seek(didRead)

dev = recovery.acquire_device()

while True:
    nop = f.read(4)
    assert(len(nop) == 4)

    aesInput = f.read(BLOCKS_CNT*16)
    assert(len(aesInput) == BLOCKS_CNT*16)
    print(binascii.hexlify(aesInput))
    print("\n\n")
    
    lastblock = bytes([0x00]*16)
    
    aesOutput = bytes()
    
    curblockindex = 0
    while curblockindex < BLOCKS_CNT:
        input = aesInput[16*curblockindex:16*(3+curblockindex)]
        cmd = "d " +binascii.hexlify(input).decode()
        print(cmd) #DEBUG
        recovery.send_command(dev,cmd)
        rsp = dev.ctrl_transfer(0xC0, 0, 0, 0, 0x600, 30000)[0:-1]
        
        for i in range(16):
            rsp[i] ^= lastblock[i]

        print(binascii.hexlify(rsp))
        aesOutput += bytes(rsp)

        lastblock = input[-16:]


        curblockindex += int(len(rsp)/(16*2))
#        exit(1)

    print("lol--")
    print(aesOutput)
#    print(bytes(aesOutput).decode())


#DEBUG
    aesInput = f.read(BLOCKS_CNT*16)
    print(binascii.hexlify(aesInput))


    exit(1)


f.close()
