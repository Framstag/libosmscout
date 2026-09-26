#!/usr/bin/env bash
#
# Compare the parameter types of the Java client's native methods with the JNI functions that
# implement them.
#
# The JVM does not check that a JNI function reads its parameters with the types the Java
# declaration names: a wrong type reads a value from the wrong place and yields garbage instead of
# an error. That is how the magnification of render() once became ~1e288 (level 957) and aborted
# the client in a build with asserts, while a release build silently read past the cell-size table.
#
# This check compares the two source files on the host: no database, device or rendering run is
# needed. A parameter-list difference fails; a JNI function with no Java declaration is reported as
# dead bridge code (a warning, not a failure).
#
# Usage:
#   scripts/check-jni-signatures.sh [java-file] [cpp-file]
#
# Exit code: 0 when every native method has a JNI function with the same parameter types,
# 1 when a parameter list differs, 2 when an input file is missing.

set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
java_file="${1:-$root/libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java}"
cpp_file="${2:-$root/libosmscout-client-java/src/OSMScoutClient.cpp}"

for f in "$java_file" "$cpp_file"; do
  if [ ! -f "$f" ]; then
    echo "check-jni-signatures: no such file: $f" >&2
    exit 2
  fi
done

awk '
function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }

function jni_type(t,    base, arr) {
  t = trim(t)
  if (t ~ /[ \t]/) { sub(/[ \t]+[A-Za-z0-9_]+$/, "", t); t = trim(t) }   # drop the parameter name
  arr = (t ~ /\[\][ \t]*$/) ? 1 : 0
  base = t; sub(/\[\][ \t]*$/, "", base); base = trim(base)
  if (index(base, "<") > 0) { base = "Object" }                          # generics travel as objects
  if (arr) {
    if (base == "int")     { return "jintArray" }
    if (base == "long")    { return "jlongArray" }
    if (base == "double")  { return "jdoubleArray" }
    if (base == "float")   { return "jfloatArray" }
    if (base == "boolean") { return "jbooleanArray" }
    if (base == "byte")    { return "jbyteArray" }
    if (base == "short")   { return "jshortArray" }
    if (base == "char")    { return "jcharArray" }
    return "jobjectArray"
  }
  if (base == "int")     { return "jint" }
  if (base == "long")    { return "jlong" }
  if (base == "double")  { return "jdouble" }
  if (base == "float")   { return "jfloat" }
  if (base == "boolean") { return "jboolean" }
  if (base == "byte")    { return "jbyte" }
  if (base == "short")   { return "jshort" }
  if (base == "char")    { return "jchar" }
  if (base == "String")  { return "jstring" }
  if (base == "void")    { return "void" }
  return "jobject"
}

function type_list(args,    n, parts, i, out, seen, t) {
  out = ""; seen = 0
  n = split(args, parts, ",")
  for (i = 1; i <= n; i++) {
    parts[i] = trim(parts[i])
    if (parts[i] == "") { continue }
    t = jni_type(parts[i])
    out = out (seen ? "," : "") t
    seen = 1
  }
  return out
}

FNR == 1 { file++ }
{
  if (file == 1) { java_text = java_text " " $0 }
  else           { cpp_text  = cpp_text  " " $0 }
}

END {
  # --- Java: "native <ret> <name>(<args>);" ----------------------------------------------------
  while (match(java_text, /native[ \t]+[A-Za-z0-9_]+(<[^>]*>)?(\[\])?[ \t]+[A-Za-z0-9_]+[ \t]*\([^)]*\)[ \t]*;/)) {
    decl = substr(java_text, RSTART, RLENGTH)
    java_text = substr(java_text, RSTART + RLENGTH)

    sub(/^native[ \t]+/, "", decl)
    open = index(decl, "(")
    prefix = substr(decl, 1, open - 1)
    split(prefix, words, /[ \t]+/)
    name = words[length(words)]
    args = substr(decl, open + 1)
    sub(/\)[ \t]*;[ \t]*$/, "", args)

    key = name "\t" type_list(args)
    java_keys[key] = 1
    java_names[name] = 1
    java_count++
  }

  # --- C++: "Java_..._<name>(<args>)" ----------------------------------------------------------
  while (match(cpp_text, /Java_com_framstag_libosmscout_client_OSMScoutClient_[A-Za-z0-9_]+\([^)]*\)/)) {
    decl = substr(cpp_text, RSTART, RLENGTH)
    cpp_text = substr(cpp_text, RSTART + RLENGTH)

    sub(/^Java_com_framstag_libosmscout_client_OSMScoutClient_/, "", decl)
    open = index(decl, "(")
    name = substr(decl, 1, open - 1)
    sub(/__.*$/, "", name)                                    # overload mangling carries the types
    args = substr(decl, open + 1)
    sub(/\)[ \t]*$/, "", args)

    out = ""; seen = 0; params = 0
    n = split(args, parts, ",")
    for (i = 1; i <= n; i++) {
      p = trim(parts[i])
      if (p == "") { continue }
      params++
      if (params <= 2) { continue }                           # JNIEnv* and jobject
      sub(/[ \t]+[A-Za-z0-9_]+$/, "", p)
      p = trim(p)
      out = out (seen ? "," : "") p
      seen = 1
    }

    key = name "\t" out
    cpp_keys[key] = 1
    cpp_any[name] = (name in cpp_any) ? cpp_any[name] " | " out : out
  }

  status = 0
  for (key in java_keys) {
    if (key in cpp_keys) { continue }
    split(key, kv, "\t")
    status = 1
    printf "MISMATCH: %s\n  java: %s\n", kv[1], kv[2]
    if (kv[1] in cpp_any) {
      printf "  cpp : %s\n", cpp_any[kv[1]]
    } else {
      printf "  cpp : <no JNI function with this name>\n"
    }
  }

  dead = 0
  for (key in cpp_keys) {
    if (key in java_keys) { continue }
    split(key, kv, "\t")
    if (kv[1] in java_names) { continue }                     # another overload of a declared method
    dead++
    printf "WARNING: JNI function without a Java declaration: %s (%s)\n", kv[1], kv[2]
  }

  if (status == 0) {
    printf "JNI signatures match: %d native declarations checked", java_count
    if (dead > 0) { printf ", %d dead JNI functions reported above", dead }
    printf ".\n"
  }
  exit status
}
' "$java_file" "$cpp_file"
