#!/bin/sh
# Phase D gate for fix-cmake-jni-header-dir, reproducible.
# Run with bash:  bash openspec/changes/fix-cmake-jni-header-dir/evidence/gate-run.sh 2>&1 | tee <log>
# (an earlier run of this check inside the interactive shell split the header list as one word and could
#  not read PIPESTATUS; this script uses only POSIX constructs and prints every rc explicitly)
set -u
cd /home/tim/projects/libosmscout
H=build/libosmscout-client-java/java/headers
S=$H/com_framstag_libosmscout_client_OSMScoutClient_removedMethod.h
CUR="com_framstag_libosmscout_client_MapDownloadManager.h
com_framstag_libosmscout_client_NavigationController.h
com_framstag_libosmscout_client_OSMScoutClientBuilder.h
com_framstag_libosmscout_client_OSMScoutClient.h"
echo "sentinel stale JNI header, no source declares this native method" > "$S"
rm -f "$H/com_framstag_libosmscout_client_OSMScoutClient.h"
echo "--- cmake --build build --target java_compile"
cmake --build build --target java_compile
echo "java_compile rc=$?"
echo "--- headers after the build:"
ls -1 "$H"
if [ -e "$S" ]; then echo "a FAIL: stale sentinel survives"; else echo "a OK: stale sentinel gone"; fi
missing=0
for h in $CUR; do
  [ -f "$H/$h" ] || { echo "b FAIL: missing $h"; missing=1; }
done
[ "$missing" = 0 ] && echo "b OK: all four headers of the current sources exist"
echo "--- touch libosmscout-client-java/src/OSMScoutClient.cpp (mtime only), then cmake --build build -j $(nproc)"
touch libosmscout-client-java/src/OSMScoutClient.cpp
cmake --build build -j "$(nproc)" > build/gate-build.out 2>&1
echo "full build rc=$?"
echo "error lines: $(grep -cE 'error:|Error [0-9]' build/gate-build.out)"
echo "--- last lines of the full build:"
tail -3 build/gate-build.out
ls -l build/libosmscout-client-java/libosmscout_client_java.so
echo "--- touched source content unchanged:"
git diff --stat -- libosmscout-client-java/src/OSMScoutClient.cpp
echo "(empty above = no content change)"
