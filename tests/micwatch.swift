// micwatch: lists the processes that record from a microphone right now, as Core Audio reports them
// (macOS 14.2 or newer). It never opens the microphone itself. tests/games.zsh calls it while games
// run and stops them if a Wine process records.
//   micwatch    one line per recording process: pid and path; exit code 1 if one of them is Wine's
// Build: xcrun swiftc -O -o tests/micwatch tests/micwatch.swift
import CoreAudio
import Foundation

func address(_ selector: AudioObjectPropertySelector) -> AudioObjectPropertyAddress {
    AudioObjectPropertyAddress(mSelector: selector, mScope: kAudioObjectPropertyScopeGlobal,
                               mElement: kAudioObjectPropertyElementMain)
}

func value<T: FixedWidthInteger>(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, _ initial: T) -> T? {
    var where_ = address(selector)
    var result = initial
    var size = UInt32(MemoryLayout<T>.size)
    let status = withUnsafeMutableBytes(of: &result) {
        AudioObjectGetPropertyData(object, &where_, 0, nil, &size, $0.baseAddress!)
    }
    return status == noErr ? result : nil
}

var list = address(kAudioHardwarePropertyProcessObjectList)
var size: UInt32 = 0
guard AudioObjectGetPropertyDataSize(AudioObjectID(kAudioObjectSystemObject), &list, 0, nil, &size) == noErr else {
    print("micwatch: Core Audio lists no processes")
    exit(2)
}
var processes = [AudioObjectID](repeating: 0, count: Int(size) / MemoryLayout<AudioObjectID>.size)
guard AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject), &list, 0, nil, &size, &processes) == noErr else {
    print("micwatch: Core Audio lists no processes")
    exit(2)
}

var wine = false
for process in processes where value(process, kAudioProcessPropertyIsRunningInput, UInt32(0)) == 1 {
    let pid = value(process, kAudioProcessPropertyPID, pid_t(0)) ?? 0
    var path = [CChar](repeating: 0, count: 4096)
    let name = proc_pidpath(pid, &path, UInt32(path.count)) > 0 ? String(cString: path) : "?"
    print("\(pid) \(name)")
    if name.hasSuffix("/wine") || name.contains("/lib/wine/") { wine = true }
}
exit(wine ? 1 : 0)
