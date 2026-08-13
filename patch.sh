#!/bin/sh
#
# patch.sh - make a Source SDK 2006 dedicated server (Insurgency) run on
# modern Linux with Steam master-server registration.
#
# Usage:  ./patch.sh [server_dir] [steamcmd_dir]
#
#   server_dir defaults to the current directory.
#   steamcmd_dir is your SteamCMD installation containing linux32/
#   (steamclient.so, libtier0_s.so, libvstdlib_s.so). If omitted, the
#   script looks in ./steamcmd, ~/steamcmd and /opt/steamcmd.
#   The script is idempotent: running it twice is safe.
#
# What it does:
#   1. chmod +x the binaries and 32-bit .so files.
#   2. Installs the sdk06-steam2less plugin (libfixes_i486.so) + fixes.vdf.
#      (https://github.com/caatge/sdk06-steam2less)
#   3. Binary-patches libsteamvalidateuseridtickets_i486.so so the server
#      returns a real Steam2 session key instead of NULL.
#   4. Replaces steamclient/tier0_s/vstdlib_s with the current SteamCMD
#      32-bit Steam client libraries (keeps backups under bin/.prepatch).
#   5. Adds the libsteamvalidateuseridtickets.so -> _i486.so symlink.
#   6. Writes steam_appid.txt (215).
#   7. Normalizes cfg/valve.rc and creates cfg/autoexec.cfg.
#   8. Copies hl2/materials/Debug/debugempty.* to the lowercase path Linux needs.
#   9. Installs libmemfix_i486.so and preloads it from srcds_run, fixing the
#      CPU/glibc memcpy corruption that mangles command lines and config files.

set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SERVER_DIR="${1:-.}"
SERVER_DIR=$(CDPATH= cd -- "$SERVER_DIR" && pwd)

ASSETS="$SCRIPT_DIR/patch_assets"

say()  { printf '%s\n' "$*"; }
die()  { say "ERROR: $*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

# ---------------------------------------------------------------- locate files
pick() {
    # pick <name> <primary> <fallback...>
    _name=$1; shift
    for _f in "$@"; do
        if [ -f "$_f" ]; then
            printf '%s\n' "$_f"
            return 0
        fi
    done
    die "cannot find $_name (looked in patch_assets/ and fallback paths)"
}

pick_opt() {
    # pick <name> <primary> <fallback...>; empty if nothing found
    _name=$1; shift
    for _f in "$@"; do
        [ -n "$_f" ] || continue
        if [ -f "$_f" ]; then
            printf '%s\n' "$_f"
            return 0
        fi
    done
    return 1
}


# libfixes_i486.so ships in patch_assets/ (built from
# https://github.com/caatge/sdk06-steam2less). LIBFIXES_PATH can point at a
# custom build; otherwise we try to clone and build it below.
PLUGIN=$(pick_opt "libfixes_i486.so" \
    "$ASSETS/libfixes_i486.so" \
    "${LIBFIXES_PATH:-}")

# Steam client libraries come from the user's SteamCMD install.
# Pass it as the second argument or via STEAMCMD_DIR=/path ./patch.sh.
# It must contain linux32/steamclient.so, libtier0_s.so and libvstdlib_s.so.
STEAMCMD_DIR="${2:-${STEAMCMD_DIR:-}}"
if [ -n "$STEAMCMD_DIR" ] && [ -f "$STEAMCMD_DIR/linux32/steamclient.so" ]; then
    SCDIR="$STEAMCMD_DIR"
elif [ -f "$SCRIPT_DIR/steamcmd/linux32/steamclient.so" ]; then
    SCDIR="$SCRIPT_DIR/steamcmd"
elif [ -f "$HOME/steamcmd/linux32/steamclient.so" ]; then
    SCDIR="$HOME/steamcmd"
elif [ -f "/opt/steamcmd/linux32/steamclient.so" ]; then
    SCDIR="/opt/steamcmd"
else
    die "SteamCMD linux32 not found. Pass it: ./patch.sh <server_dir> /path/to/steamcmd"
fi

STEAMCLIENT=$(pick "steamclient_i486.so" \
    "$ASSETS/steamclient_i486.so" \
    "$SCDIR/linux32/steamclient.so")

TIER0=$(pick "libtier0_s.so" \
    "$ASSETS/libtier0_s.so" \
    "$SCDIR/linux32/libtier0_s.so")

VSTDLIB=$(pick "libvstdlib_s.so" \
    "$ASSETS/libvstdlib_s.so" \
    "$SCDIR/linux32/libvstdlib_s.so")

MEMFIX=$(pick "libmemfix_i486.so" \
    "$ASSETS/libmemfix_i486.so")

# libfixes_i486.so is not part of SteamCMD; it is built from
# https://github.com/caatge/sdk06-steam2less. If it was not provided,
# try to clone and build it automatically.
if [ -z "$PLUGIN" ]; then
    if have git && have cmake && have g++; then
        BUILD_DIR="$SCRIPT_DIR/.sdk06-steam2less"
        if [ ! -d "$BUILD_DIR" ]; then
            git clone --depth 1 https://github.com/caatge/sdk06-steam2less "$BUILD_DIR" || die "git clone sdk06-steam2less failed"
        fi
        ( cd "$BUILD_DIR" && mkdir -p build && cd build && cmake .. && cmake --build . ) || die "building sdk06-steam2less failed"
        PLUGIN="$BUILD_DIR/build/libfixes_i486.so"
    else
        die "libfixes_i486.so not found; provide it in patch_assets/ or install git, cmake, g++ and g++-multilib (Debian/Ubuntu) to build it"
    fi
fi

# ------------------------------------------------------------------- sanity
[ -d "$SERVER_DIR" ] || die "server directory not found: $SERVER_DIR"
[ -f "$SERVER_DIR/srcds_i686" ] || die "$SERVER_DIR doesn't look like a server (no srcds_i686)"
[ -d "$SERVER_DIR/insurgency" ] || die "$SERVER_DIR doesn't look like a server (no insurgency/)"
[ -d "$SERVER_DIR/bin" ] || die "$SERVER_DIR/bin missing"

cd "$SERVER_DIR" || die "cannot cd into $SERVER_DIR"

say "Patching server: $SERVER_DIR"

# ------------------------------------------------------ host compatibility
ARCH=$(uname -m 2>/dev/null || true)
case "$ARCH" in
    x86_64|i686|i386|x86) : ;;
    *)
        say "WARNING: host arch is '$ARCH'; srcds is a 32-bit x86 binary and"
        say "         will need an x86 host or emulation to run." ;;
esac

if [ ! -e /lib/ld-linux.so.2 ]; then
    say "WARNING: 32-bit x86 loader (/lib/ld-linux.so.2) not found."
    say "         Install 32-bit support (e.g. dpkg --add-architecture i386 &&"
    say "         apt install libc6:i386 libstdc++6:i386) or run in a 32-bit container."
fi

# -------------------------------------------------------------- executables
chmod +x srcds_amd srcds_i486 srcds_i686 srcds_run 2>/dev/null || true
chmod +x bin/*.so 2>/dev/null || true
say "[1/9] binaries are executable"

# ------------------------------------------------- plugin (sdk06-steam2less)
mkdir -p insurgency/addons
cp -f "$PLUGIN" insurgency/addons/libfixes_i486.so
chmod +x insurgency/addons/libfixes_i486.so
cat > insurgency/addons/fixes.vdf <<'EOF'
"Plugin"
{
	"file"	"insurgency/addons/libfixes_i486.so"
}
EOF
say "[2/9] sdk06-steam2less plugin installed (insurgency/addons/libfixes_i486.so)"

# --------------------------------------- patch Steam2 encryption key getter
if ! have python3; then
    die "python3 is required for the binary patch"
fi
python3 - bin/libsteamvalidateuseridtickets_i486.so <<'PY'
import sys

path = sys.argv[1]
off = 0xB0050
orig = bytes.fromhex(
    "e85719ffff84c0741785f674138b93840e00008b0289068b42045b5e5dc389f6"
)
patched = bytes.fromhex(
    "85f6741cc706080000008d83840e0000eb0890909090909090905b5e5dc389f6"
)

with open(path, "rb") as f:
    data = bytearray(f.read())

if data[off:off + len(patched)] == patched:
    print("already patched")
elif data[off:off + len(orig)] == orig:
    data[off:off + len(patched)] = patched
    with open(path, "wb") as f:
        f.write(data)
    print("patched")
else:
    sys.exit("unexpected bytes at validator+0xB0050; refusing to patch")
PY
rc=$?
[ $rc -eq 0 ] || die "validator patch failed (exit $rc)"

# backup only the first time
if [ ! -f bin/libsteamvalidateuseridtickets_i486.so.orig ]; then
    cp -p bin/libsteamvalidateuseridtickets_i486.so bin/libsteamvalidateuseridtickets_i486.so.orig
fi
ln -sfn libsteamvalidateuseridtickets_i486.so bin/libsteamvalidateuseridtickets.so
say "[3/9] Steam2 encryption-key getter patched (backup kept)"

# --------------------------------------------------------- modern Steam libs
mkdir -p bin/.prepatch
if [ ! -f bin/.prepatch/steamclient_i486.so.orig ]; then
    cp -p bin/steamclient_i486.so  bin/.prepatch/steamclient_i486.so.orig
fi
if [ ! -f bin/.prepatch/libtier0_s.so.orig ]; then
    cp -p bin/libtier0_s.so        bin/.prepatch/libtier0_s.so.orig
fi
if [ ! -f bin/.prepatch/libvstdlib_s.so.orig ]; then
    cp -p bin/libvstdlib_s.so      bin/.prepatch/libvstdlib_s.so.orig
fi
cp -f "$STEAMCLIENT" bin/steamclient_i486.so
cp -f "$TIER0"        bin/libtier0_s.so
cp -f "$VSTDLIB"      bin/libvstdlib_s.so
chmod +x bin/steamclient_i486.so bin/libtier0_s.so bin/libvstdlib_s.so
say "[4/9] modern Steam client libs installed (old copies in bin/.prepatch)"

# ------------------------------------------------------------------ appid
printf '215' > steam_appid.txt
say "[5/9] steam_appid.txt = 215"

# ---------------------------------------------------------- cfg/valve.rc
mkdir -p insurgency/cfg
cat > insurgency/cfg/valve.rc <<'EOF'
exec autoexec.cfg
stuffcmds
EOF
if [ ! -f insurgency/cfg/autoexec.cfg ]; then
    printf '// user autoexec\n' > insurgency/cfg/autoexec.cfg
fi
say "[6/9] cfg/valve.rc + cfg/autoexec.cfg normalized"

# ------------------------------------------------- debugempty material
if [ -f hl2/materials/Debug/debugempty.vmt ] && [ -f hl2/materials/Debug/debugempty.vtf ]; then
    mkdir -p hl2/materials/debug
    cp -f hl2/materials/Debug/debugempty.vmt hl2/materials/debug/debugempty.vmt
    cp -f hl2/materials/Debug/debugempty.vtf hl2/materials/debug/debugempty.vtf
    say "[7/9] hl2/materials/debug/debugempty.* installed"
else
    say "[7/9] WARN: hl2/materials/Debug/debugempty.* not found, skipping"
fi

# ------------------------------------------------------ CPU/memcpy fix
# Only apply on Intel CPUs that exhibit the glibc memcpy corruption
# (Nehalem and later, i.e. SSE4.2/POPCNT era). AMD and pre-Nehalem Intel
# are unaffected. FORCE_MEMFIX=1 overrides the check.
CPU_VENDOR=""
if [ -r /proc/cpuinfo ]; then
    CPU_VENDOR=$(grep -m1 '^vendor_id' /proc/cpuinfo | awk '{print $3}')
fi
CPU_AFFECTED=0
if [ "$CPU_VENDOR" = "GenuineIntel" ]; then
    if grep -m1 '^flags' /proc/cpuinfo | grep -q 'sse4_2'; then
        CPU_AFFECTED=1
    fi
fi
if [ "${FORCE_MEMFIX:-0}" = "1" ]; then
    CPU_AFFECTED=1
fi

if [ "$CPU_AFFECTED" = "1" ]; then
    cp -f "$MEMFIX" bin/libmemfix_i486.so
    chmod +x bin/libmemfix_i486.so

    python3 - srcds_run <<'PY'
import sys

path = sys.argv[1]
with open(path, "rb") as f:
    lines = f.read().decode("utf-8", "surrogateescape").splitlines(keepends=True)

pre = 'env LD_PRELOAD=./bin/libmemfix_i486.so${LD_PRELOAD:+:$LD_PRELOAD} '
changed = False
for i, line in enumerate(lines):
    stripped = line.strip()
    if stripped == "$HL_CMD":
        lines[i] = line.replace("$HL_CMD", pre + "$HL_CMD", 1)
        changed = True
    elif stripped == "exec $HL_CMD":
        lines[i] = line.replace("exec $HL_CMD", "exec " + pre + "$HL_CMD", 1)
        changed = True

if changed:
    with open(path, "wb") as f:
        f.write("".join(lines).encode("utf-8", "surrogateescape"))
    print("srcds_run patched with LD_PRELOAD")
else:
    print("srcds_run already has LD_PRELOAD")
PY
    say "[8/9] memcpy fix installed (bin/libmemfix_i486.so + srcds_run LD_PRELOAD)"
else
    say "[8/9] memcpy fix skipped (host CPU: ${CPU_VENDOR:-unknown} - not affected)"
fi

# -------------------------------------------------------------- summary
say "[9/9] patch complete for $SERVER_DIR"
say
say "Launch it yourself, e.g.:"
say "  $SERVER_DIR/srcds_run -game insurgency -norestart +map ins_almaden +maxplayers 16 +sv_lan 0 +sv_master_legacy_mode 0"
say "Restore originals from bin/.prepatch if you ever need to."
