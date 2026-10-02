#!/usr/bin/env bash
# Testa a ponte nos 3 idiomas (C++ <-> Java <-> Python). Precisa de g++ e JDK 21+ (javac) e python3.
set -euo pipefail
cd "$(dirname "$0")/.."
T=$(mktemp -d); L="$T/link.bin"
g++ -std=c++20 -Wall -Wextra -O2 dusklight-mod/src/link.cpp dusklight-mod/src/link_selftest.cpp -o "$T/cpp"
if command -v javac >/dev/null; then JAVAC=javac; else JAVAC="java -m jdk.compiler/com.sun.tools.javac.Main"; fi
$JAVAC -d "$T/j" fabric/src/main/java/dev/tpcraft/link/*.java fabric/src/test/java/dev/tpcraft/link/LinkSelfTest.java
J="java -cp $T/j dev.tpcraft.link.LinkSelfTest"; P="python3 tools/fake_dusklight.py"
echo "C++ -> Java";    "$T/cpp" write "$L" >/dev/null; $J read "$L"
echo "Java -> C++";    $J write "$L" >/dev/null;       "$T/cpp" read "$L"
rm -f "$L"
echo "Python -> Java"; $P write --path "$L" >/dev/null; $J read "$L"
echo "Java -> Python"; $J write "$L" >/dev/null;       $P read --path "$L"
rm -f "$L"
echo "C++ -> Java input"; "$T/cpp" push-input "$L" >/dev/null; $J input-read "$L"
rm -f "$L"
echo "Python -> Java input"; $P input-write --path "$L" >/dev/null; $J input-read "$L"
rm -f "$L"
echo "C++ -> Python input"; "$T/cpp" push-input "$L" >/dev/null; $P input-read --path "$L"
rm -f "$L"
echo "Java -> C++ render (wrap)"; $P render-prime --path "$L" >/dev/null; $J render-write "$L" >/dev/null; "$T/cpp" read-render "$L"
rm -f "$L"
echo "Python -> C++ render"; $P render-write --path "$L" >/dev/null; "$T/cpp" read-render "$L"
rm -rf "$T"; echo "OK"
