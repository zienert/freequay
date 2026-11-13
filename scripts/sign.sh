#!/bin/zsh
# Signs Freequay.app and every Mach-O file inside it, from the inside out, with the hardened runtime
# and the entitlements Wine needs (app/Freequay.entitlements) and Apple's timestamp. Without an
# identity it signs with the Developer ID Application certificate in the keychain, which Apple's
# notarization asks for (scripts/notarize.sh). With "-", or when the keychain has no such certificate,
# the signature is ad hoc, as in the releases up to 0.5.0, and macOS opens the app only after Open Anyway.
# Usage: sign.sh <Freequay.app> [identity]
set -euo pipefail

ROOT=${0:A:h:h}
APP=${${1:?path to Freequay.app missing}:A}
# awk reads to the end: one that quits early kills the writer with SIGPIPE, which pipefail counts
ID=${2:-$(security find-identity -v -p codesigning | awk '/"Developer ID Application: / && !n++ {print $2}')}
if [[ -z $ID ]]; then
  print -u2 "no Developer ID Application certificate in the keychain, signing ad hoc"
  ID=-
fi
opts=(--force --options runtime --entitlements "$ROOT/app/Freequay.entitlements" --sign "$ID")
[[ $ID == - ]] || opts+=(--timestamp)

xattr -cr "$APP"  # codesign refuses Finder information and similar detritus
# Mach-O files by their first bytes: thin 64 and 32 bit, and universal
macho=()
for f in "$APP"/Contents/**/*(.N); do
  [[ $f == $APP/Contents/MacOS/${APP:t:r} ]] && continue
  case $(od -An -tx1 -N4 "$f" | tr -d ' \n') in
    cffaedfe|cefaedfe|cafebabe|cafebabf) macho+=("$f") ;;
  esac
done
# deepest first, so that every folder is signed before what contains it; codesign reports every
# replaced signature, so its messages only show when it fails, such as without the timestamp server
for f in ${(On)macho}; do
  out=$(codesign $opts "$f" 2>&1) || { print -u2 -r -- "${f#$APP/}: $out"; exit 1; }
done
codesign $opts "$APP"
codesign --verify --deep --strict "$APP"
who=$(codesign -dvv "$APP" 2>&1 | awk -F= '/^Authority=/ && !n++ {print $2}')
print "signed ${#macho} files and ${APP:t} (${who:-ad hoc})"
