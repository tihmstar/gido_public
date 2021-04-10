#!/bin/bash


#compressed, all pos first, then all blocks, but skip existing
export indir="normalized_decrypted_traces"
export outdir="break_hw8_normalized_decrypted"
time for i in 3 2 4 1 5 0 6 7; do
        sed "s.#define AES_BLOCK_SELECTOR.#define AES_BLOCK_SELECTOR $i//.g" kernels_sel.cl >kernels.cl;
        echo $i;
        mkdir ../"$outdir"_"$i";
        if [ -f "../$outdir"_"$i/CORR-powerModel_ATK_0-KEYGUESS_0" ]; then
                echo "../$outdir"_"$i already exists, skipping...";
                continue;
        fi;
        ./break_corr_opencl ../"$indir" ../"$outdir"_"$i" 0 16 || break;
done

