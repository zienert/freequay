#!/bin/zsh
# Has Apple's notary service check Freequay.app or its disk image and staples the ticket to it, so that
# macOS opens a download without Open Anyway, offline as well. The app and the image must be signed with
# a Developer ID (scripts/sign.sh, codesign). notarytool finds its login in the keychain under the
# profile "freequay", or the one FREEQUAY_NOTARY_PROFILE names; store it once with an App Store Connect
# API key (team key, Developer role)
#   xcrun notarytool store-credentials freequay --key AuthKey_<key ID>.p8 --key-id <key ID> --issuer <issuer ID>
# or with an app-specific password of the Apple Account (account.apple.com), which this asks for
#   xcrun notarytool store-credentials freequay --apple-id <Apple Account> --team-id <team ID>
# Usage: notarize.sh <Freequay.app|Freequay-<version>.dmg>
set -euo pipefail

FILE=${${1:?path to Freequay.app or its disk image missing}:A}
PROFILE=${FREEQUAY_NOTARY_PROFILE:-freequay}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

upload=$FILE
if [[ -d $FILE ]]; then  # an app goes up as a zip archive; the ticket is stapled to the app itself
  upload=$TMP/${FILE:t:r}.zip
  ditto -c -k --keepParent "$FILE" "$upload"
fi
print "==> notarizing ${FILE:t}, $(du -h "$upload" | cut -f1)"
xcrun notarytool submit "$upload" --keychain-profile "$PROFILE" --wait --output-format json > "$TMP/result.json" ||
  true  # the result says what went wrong
field() { plutil -extract "$1" raw -o - "$TMP/result.json" 2> /dev/null || true; }
id=$(field id)
result=$(field status)
if [[ $result != Accepted ]]; then
  [[ -z $id ]] || xcrun notarytool log "$id" --keychain-profile "$PROFILE" >&2 || true
  print -u2 "notarization of ${FILE:t} failed: ${result:-no result} ${id:+($id)}"
  cat "$TMP/result.json" >&2
  exit 1
fi
xcrun stapler staple -q "$FILE"
xcrun stapler validate -q "$FILE"
print "notarized and stapled ${FILE:t} ($id)"
