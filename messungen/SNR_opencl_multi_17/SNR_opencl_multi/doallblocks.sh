#!/bin/bash

export indir="normalized_Traces"
export outdir="results_snr"
time for i in $(seq 0 7); do
        sed "s/#define AES_BLOCK_SELECTOR 0/#define AES_BLOCK_SELECTOR $i/g" kernels_sel.cl >kernels.cl;
        echo $i;
        mkdir ../"$outdir"_"$i";
        if [ -f "../$outdir"_"$i/whatfile" ]; then
                echo "../$outdir"_"$i already exists, skipping...";
                continue;
        fi;
        ./snrtest_opencl_multi ../"$indir" ../"$outdir"_"$i" || break;
done

