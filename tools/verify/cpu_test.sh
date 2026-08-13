#!/bin/sh
# usage: cpu_test.sh <model> <baseport> <tag> [server_dir]
model="$1"; port="$2"; tag="$3"; server_dir="${4:-.}"
log="/tmp/cputest_${tag}.log"
cd "$server_dir" || exit 99
timeout -k 4 22 script -q -c "env LD_LIBRARY_PATH=.:bin qemu-i386-static -cpu $model ./srcds_i686 -game insurgency -port $port +clientport $((port+1)) +tv_port $((port+2)) +map ins_sinjar +maxplayers 2 -insecure -tickrate 20 -usercon +sv_lan 1 -nomaster" "$log" >/dev/null 2>&1
echo "=== $model (tag=$tag) exit=$? ==="
grep -aE "couldn't exec|Unknown command|No such map|map load failed|Console initialized|Executing Server Config|Network:|Precache|qemu: uncaught|Illegal instruction|signal" "$log" | head -30
