#!/usr/bin/env bash
# lang_prefix.sh <tag> <out.o> <objects...>: one translation mod as one
# object whose defined symbols all start with <tag>_ (patches/Makefile,
# LANG_MODS_DIR), so the four languages, which define the same names (the
# game functions they patch among them), link side by side. The game's
# symbols they use stay as they are (undefined here).
set -euo pipefail
tag=$1; out=$2; shift 2
LD=${LD:-ld.lld}; NM=${NM:-llvm-nm}; OBJCOPY=${OBJCOPY:-llvm-objcopy}
$LD -r "$@" -o "$out.all"
$NM --defined-only -g "$out.all" | awk -v t="$tag" '{ print $3 " " t "_" $3 }' > "$out.syms"
$OBJCOPY --redefine-syms="$out.syms" "$out.all" "$out"
rm -f "$out.all"
