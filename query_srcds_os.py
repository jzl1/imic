#!/usr/bin/env python3
"""Query a Source engine dedicated server (srcds) for its OS type.

Usage:
    ./query_srcds_os.py <host> [port] [--quiet] [--timeout SECONDS]

The OS type comes from the server's A2S_INFO response, so no Steam
authentication or RCON access is required.
"""

import argparse
import socket
import struct
import sys


A2S_INFO = b"\xff\xff\xff\xffTSource Engine Query\x00"

OS_NAMES = {
    "l": "Linux",
    "w": "Windows",
    "m": "Mac OS X",
    "o": "Mac OS X (older)",
}

DEDICATED_NAMES = {
    "d": "Dedicated",
    "l": "Listen",
    "p": "SourceTV relay",
}


def read_cstring(buf, offset):
    end = buf.find(b"\x00", offset)
    if end == -1:
        raise ValueError("malformed response: unterminated string")
    raw = buf[offset:end]
    try:
        value = raw.decode("utf-8")
    except UnicodeDecodeError:
        # Old servers sometimes send CP-1252/Latin-1 names.
        value = raw.decode("latin-1")
    return value, end + 1


def recv_response(sock):
    data, _ = sock.recvfrom(4096)
    if len(data) < 5 or data[:4] != b"\xff\xff\xff\xff":
        raise ValueError("not a valid server query response")
    return data


def query_info(host, port, timeout):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(timeout)
    try:
        sock.sendto(A2S_INFO, (host, port))
        data = recv_response(sock)

        # Some servers (including this SDK 2006 setup) answer A2S_INFO with a
        # challenge packet first: FF FF FF FF 'A' + 4-byte challenge.
        # Resend the query with that challenge appended to get the real info.
        if data[4] == ord("A") and len(data) >= 9:
            challenge = data[5:9]
            sock.sendto(A2S_INFO + challenge, (host, port))
            data = recv_response(sock)
    finally:
        sock.close()

    if len(data) < 6 or data[4] != ord("I"):
        raise ValueError("not a valid A2S_INFO response")

    off = 5
    protocol = data[off]
    off += 1

    name, off = read_cstring(data, off)
    map_name, off = read_cstring(data, off)
    folder, off = read_cstring(data, off)
    game, off = read_cstring(data, off)
    app_id = struct.unpack_from("<H", data, off)[0]
    off += 2

    players, max_players, bots = data[off], data[off + 1], data[off + 2]
    off += 3

    dedicated = chr(data[off])
    off += 1
    os_byte = chr(data[off])
    off += 1
    password_required = bool(data[off])
    off += 1
    vac_secure = bool(data[off])
    off += 1
    version, off = read_cstring(data, off)

    edf = None
    if off < len(data):
        edf = data[off]

    return {
        "protocol": protocol,
        "name": name,
        "map": map_name,
        "folder": folder,
        "game": game,
        "app_id": app_id,
        "players": players,
        "max_players": max_players,
        "bots": bots,
        "dedicated": dedicated,
        "os_byte": os_byte,
        "os": OS_NAMES.get(os_byte, f"Unknown ({os_byte!r})"),
        "password_required": password_required,
        "vac_secure": vac_secure,
        "version": version,
        "edf": edf,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Fetch the OS type of a remote srcds server."
    )
    parser.add_argument("host", help="server IP or hostname")
    parser.add_argument("port", nargs="?", type=int, default=27015, help="server port (default: 27015)")
    parser.add_argument("--quiet", action="store_true", help="print only the OS type")
    parser.add_argument("--timeout", type=float, default=5.0, help="UDP timeout in seconds (default: 5)")
    args = parser.parse_args(argv)

    try:
        info = query_info(args.host, args.port, args.timeout)
    except (socket.timeout, TimeoutError):
        print(f"ERROR: no response from {args.host}:{args.port} (A2S_INFO timed out)", file=sys.stderr)
        return 1
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if args.quiet:
        print(info["os"])
        return 0

    print(f"Server   : {args.host}:{args.port}")
    print(f"Name     : {info['name']}")
    print(f"Game     : {info['game']} (folder: {info['folder']}, appid {info['app_id']})")
    print(f"Map      : {info['map']}")
    print(f"Players  : {info['players']}/{info['max_players']} ({info['bots']} bots)")
    print(f"Type     : {DEDICATED_NAMES.get(info['dedicated'], info['dedicated'])}")
    print(f"OS       : {info['os']}")
    print(f"Version  : {info['version']}")
    print(f"Password : {'yes' if info['password_required'] else 'no'}")
    print(f"VAC      : {'yes' if info['vac_secure'] else 'no'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
