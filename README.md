# ifshow

Small, simple CLI to list local network interfaces and their IP addresses. Output is grouped per interface with clear bullets and includes netmasks in address/prefix form (IPv4 also shows dotted mask).

## Features
- Lists IPv4 and IPv6 addresses per interface
- Shows netmask as `address/prefix` and dotted mask for IPv4
- Simple flags: `-a` for all, `-i <name>` for a specific interface
- Minimal, readable output suitable for scripting

## Build
Prerequisites: a C compiler with POSIX networking headers (`getifaddrs`, `arpa/inet.h`). Works on Linux and macOS.

Build locally:

```
gcc -Wall -Wextra -O2 -o ifshow ifshow.c
```

Run:

```
./ifshow -a
./ifshow -i eth0
```

## Usage

```
Usage:
  ifshow -a                     # Show all interfaces
  ifshow -i <interface_name>    # Show specific interface

Examples:
  ifshow -a
  ifshow -i eth0

Notes:
  Addresses include netmask as address/prefix.
  IPv4 also shows dotted mask in parentheses.
```

Example output:

```
lo:
 - 127.0.0.1/8 (255.0.0.0)
 - ::1/128

eth0:
 - 192.168.1.10/24 (255.255.255.0)
 - fe80::215:5dff:fee2:af41%eth0/64
```

## Releases

On every push to `main`, GitHub Actions builds a Linux x86_64 executable and publishes a GitHub Release via semantic-release.

Release assets:
- `ifshow-linux-x86_64` (executable)
- `ifshow-linux-x86_64.tgz` (tarball)

Download and use (example):

```
curl -L -o ifshow https://github.com/soaoaos/ifshow/releases/download/latest/ifshow-linux-x86_64
chmod +x ifshow
./ifshow -a
```

## Versioning (semantic-release)

This project uses semantic-release to automate versioning and changelogs. Version bumps are determined by Conventional Commits in merge history:
- `feat:` → minor (or major if breaking via `!`)
- `fix:` → patch
- `perf:` → patch
- `docs:`, `chore:`, `refactor:`, etc. → no release unless marked breaking (`!`)

Configuration: `.releaserc.json` at repo root (the `ifshow` folder).

## Development Notes

- Requires `getifaddrs` (available on Linux, BSD, macOS). Not supported on Windows without compatibility layers.
- IPv4 displays `address/prefix (dotted-mask)`; IPv6 displays `address/prefix`.
- Output format is intentionally simple for ease of parsing.

## Contributing

1. Create a feature branch.
2. Follow Conventional Commits for messages (e.g., `feat: add XYZ`, `fix: correct ABC`).
3. Build and test locally: `gcc -Wall -Wextra -O2 -o ifshow ifshow.c` and run `./ifshow -a`.
4. Open a pull request.


Written by ChatGPT, this project is school project for IMT Mines Alès.