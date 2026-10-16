# Lists the standard-library includes that a file places after the start of its project-include block
# (the own header of a .cpp counts as the first project include; a .h has none).
#
# Rule: guidelines/CodeStyles.md, "Include Order" - standard library (step 2) before project headers (step 4).
# Usage: awk -f out-of-order-includes.awk $(find libosmscout libosmscout-map libosmscout-import libosmscout-client libosmscout-gpx libosmscout-test -name '*.cpp' -o -name '*.h')
FNR==1 { pc=0 }
/^[ \t]*#include/ {
  s=$0; gsub(/[ \t]+/," ",s)
  if (s ~ /^#include </) {
    if (s ~ /<osmscout/) { pc++ }
    else { limit = (FILENAME ~ /\.h$/) ? 1 : 2; if (pc >= limit) bad[FILENAME] = bad[FILENAME] " " FNR":"s }
  }
}
END { n=0; for (f in bad) { print f ":" bad[f]; n++ } print "VIOLATING_FILES=" n > "/dev/stderr" }
