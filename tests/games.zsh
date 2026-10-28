#!/bin/zsh
# games: the release check behind the table "What runs" in the README. Starts the games of
# tests/games.txt that are installed in a bottle, one after the other, and notes for each whether a
# window of it came, whether a window was still open some minutes later and whether a crash dialog
# showed. It reads lists of windows (tests/winlist.exe), never the screen. No program may record from
# the microphone: the run refuses to start while a rule allows it, and stops every program at once if
# tests/micwatch reports a Wine process that records. Games come to the front, so start it only when
# nobody needs the Mac. Steam has to be installed and signed in, the games installed through it.
# Build first: tests/winlist.exe (see its first comment) and
#   xcrun swiftc -O -o tests/micwatch tests/micwatch.swift
# Usage: tests/games.zsh <bottle> [seconds] [list]   seconds each game stays open after its first
#                                                    window, 180 by default; writes logs/games-<time>.md
set -euo pipefail
zmodload zsh/datetime

ROOT=${0:A:h:h}
BOTTLE=${1:?usage: tests/games.zsh <bottle> [seconds] [list]}
OPEN=${2:-180}
LIST=${3:-$ROOT/tests/games.txt}
FQ=$ROOT/scripts/freequay
DIR="$HOME/Library/Application Support/freequay/Bottles/$BOTTLE"
STEAM="$DIR/drive_c/Program Files (x86)/Steam"
WINLIST=$ROOT/tests/winlist.exe
MICWATCH=$ROOT/tests/micwatch
WAIT=120  # seconds for the first window
STAMP=$(strftime %Y-%m-%d-%H%M $EPOCHSECONDS)
OUT=$ROOT/logs/games-$STAMP.md
LOG=$ROOT/logs/games-$STAMP.log  # what Wine, Steam and the launcher print

[[ -d $DIR ]] || { print -u2 "no bottle $BOTTLE"; exit 1; }
for f in $WINLIST $MICWATCH $LIST; do
  [[ -e $f ]] || { print -u2 "missing $f, build it as the first comment of tests/games.zsh says"; exit 1; }
done
mic=$(grep -H -- 'FREEQUAY_MICROPHONE=1' "$DIR/compat.txt"(N) $ROOT/compat/*.txt(N) \
  "$HOME/Library/Application Support/freequay/compat/"*.txt(N) /dev/null || true)
[[ -z $mic ]] || { print -u2 "a rule allows the microphone, remove it before the run:\n$mic"; exit 1; }
mkdir -p $ROOT/logs

# Steam, its helpers and Wine's own programs; any other window belongs to the game
HELPERS=(steam.exe steamwebhelper.exe steamservice.exe steamerrorreporter.exe gameoverlayui.exe explorer.exe
  winlist.exe tasklist.exe taskkill.exe services.exe winedevice.exe plugplay.exe svchost.exe rpcss.exe
  conhost.exe start.exe cmd.exe tabtip.exe '?')
# what comes up when a program crashed: Wine's dialog and the crash reporters of the engines
CRASHERS=(winedbg.exe unitycrashhandler64.exe unitycrashhandler32.exe crashreportclient.exe werfault.exe)

# visible top-level windows as "exe<TAB>WxH<TAB>class<TAB>title"
windows() {
  local line
  "$FQ" wine "$BOTTLE" "$WINLIST" 2>/dev/null | while IFS= read -r line; do
    [[ $line =~ '^(.+) ([0-9]+)x([0-9]+) @-?[0-9]+,-?[0-9]+ \[([^]]*)\] ?(.*)$' ]] || continue
    print -r -- "$match[1]"$'\t'"$match[2]x$match[3]"$'\t'"$match[4]"$'\t'"$match[5]"
  done
}

# the windows of the game, at least 200×150, and the crash dialogs, from one list of windows
split_windows() {
  game=() crash=()
  local line exe size
  for line in ${(f)"$(windows)"}; do
    [[ -n $line ]] || continue
    exe=${line%%$'\t'*} size=${${line#*$'\t'}%%$'\t'*}
    if (( ${CRASHERS[(Ie)${exe:l}]} )); then crash+=("$line")
    elif (( ! ${HELPERS[(Ie)${exe:l}]} )) && (( ${size%x*} >= 200 && ${size#*x} >= 150 )); then game+=("$line")
    fi
  done
}

running() { "$FQ" wine "$BOTTLE" tasklist 2>/dev/null | grep -q -i -- "^$1 "; }

# stops everything if a Wine process records from the microphone
mic_check() {
  local recording
  if ! recording=$("$MICWATCH" 2>&1); then
    "$FQ" kill "$BOTTLE" > /dev/null 2>&1 || true
    print -r -- $'\n'"Stopped: a Wine process recorded from the microphone or Core Audio could not tell ($recording)." >> $OUT
    print -u2 "stopped all programs in $BOTTLE: microphone ($recording)"
    exit 2
  fi
}

# WM_CLOSE to the programs of the game, by force after 10 seconds, then the crash dialogs
stop_game() {
  local exe
  for exe in ${(u)@}; do "$FQ" wine "$BOTTLE" taskkill /IM "$exe" >> $LOG 2>&1 || true; done
  sleep 10
  for exe in ${(u)@} $CRASHERS; do
    running "$exe" && { "$FQ" wine "$BOTTLE" taskkill /F /IM "$exe" >> $LOG 2>&1 || true; }
  done
}

row() { # game, Steam app, result, first window, programs
  print -r -- "| $1 | $2 | $3 | $4 | $5 |" >> $OUT
  print -r -- "$1: $3"
}

wine_version=$("$ROOT/dist/wine/bin/wine" --version 2>/dev/null || print unknown)
{
  print "# Games in Freequay, $(strftime '%Y-%m-%d %H:%M' $EPOCHSECONDS)"
  print
  print "Freequay $(git -C $ROOT describe --tags --always --dirty 2>/dev/null || print unknown) ($wine_version)," \
    "$(sysctl -n machdep.cpu.brand_string), $(( $(sysctl -n hw.memsize) >> 30 )) GB, macOS $(sw_vers -productVersion)." \
    "Bottle $BOTTLE; each game stays open $OPEN s after its first window. Windows come from lists of windows, not from the screen."
  print
  print "| Game | Steam app | Result | First window | Programs |"
  print "|---|---|---|---|---|"
} > $OUT

if grep -q -E '^[0-9]+'$'\t' $LIST && ! running steam.exe; then
  print "starting Steam"
  "$FQ" run "$BOTTLE" "$STEAM/steam.exe" >> $LOG 2>&1 &
  for i in {1..36}; do sleep 5; mic_check; running steamwebhelper.exe && break; done
  sleep 30  # Steam signs in and loads the library
fi

while IFS=$'\t' read -r -u 3 target name kind args; do
  [[ -z $target || $target == \#* ]] && continue
  if [[ $target == <-> ]]; then
    app=$target run=steam:$target
    [[ -f "$STEAM/steamapps/appmanifest_$target.acf" ]] || { row "$name" "$app" "not installed" "" ""; continue; }
  else
    [[ $target == /* ]] || target=$DIR/$target
    app="program" run=$target
    [[ -f $target ]] || { row "$name" "$app" "not found" "" ""; continue; }
  fi
  print "starting $name"
  start=$EPOCHSECONDS first="" seen=() result=""
  "$FQ" run "$BOTTLE" $run ${(z)args} >> $LOG 2>&1 &
  launcher=$!
  while (( EPOCHSECONDS - start < WAIT )); do
    sleep 5; mic_check; split_windows
    (( $#game || $#crash )) && break
  done
  if (( $#crash )); then
    result="crash dialog after $((EPOCHSECONDS - start)) s"
  elif (( ! $#game )); then
    result="no window within $WAIT s"
  else
    first="$((EPOCHSECONDS - start)) s" opened=$EPOCHSECONDS
    while true; do
      seen+=(${game%%$'\t'*})
      (( EPOCHSECONDS - opened >= OPEN )) && { result="runs: a window was open after $OPEN s"; break; }
      sleep 10; mic_check; split_windows
      (( $#crash )) && { result="crash dialog after $((EPOCHSECONDS - start)) s"; break; }
      (( $#game )) || { result="its window closed after $((EPOCHSECONDS - start)) s"; break; }
    done
  fi
  programs=(${(u)seen} ${(u)crash%%$'\t'*})
  stop_game $programs
  kill $launcher 2> /dev/null || true
  wait $launcher 2> /dev/null || true
  row "$name ($kind)" "$app" "$result" "$first" "${(j:, :)programs}"
  sleep 10  # Steam notices that the game ended
done 3< $LIST

print -r -- $'\n'"No Wine process recorded from the microphone (checked every 5 to 10 seconds)." >> $OUT
print "wrote $OUT"
