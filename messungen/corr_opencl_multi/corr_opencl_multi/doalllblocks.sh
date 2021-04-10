#!/bin/bash

#for i in $(seq 0 7); do sed "s/#define AES_BLOCK_SELECTOR 7/#define AES_BLOCK_SELECTOR $i/g" kernels_sel.cl >kernels.cl; echo $i; #mkdir .>


#compressed, all pos first, then all blocks, but skip existing
export indir="normalized_compressed3_short_filtered"
export outdir="rescorr4_normalized3_block"
time for i in 3 4; do
        sed "s/#define AES_BLOCK_SELECTOR 0/#define AES_BLOCK_SELECTOR $i/g" kernels_sel.cl >kernels.cl;
        echo $i;
        mkdir ../"$outdir"_"$i";
        if [ -f "../$outdir"_"$i/CORR-powerModel_ROUND2_XOR_NEXT_HW128" ]; then
                echo "../$outdir"_"$i already exists, skipping...";
                continue;
        fi;
        ./corr_opencl_multi ../"$indir" ../"$outdir"_"$i" || break;
done

