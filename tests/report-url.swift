// report-url: checks the addresses of the filled-in GitHub forms that Report on GitHub opens
// (app/Report.swift), without a browser: every field arrives with exactly the text that went in,
// + and & survive, the user name stays out of messages. Exits with 1 on the first failed check.
// Build and run from the repository: xcrun swiftc -parse-as-library -o build/report-url app/Report.swift tests/report-url.swift && build/report-url
import Foundation

@main
struct ReportURLTest {
    static var failed = 0

    static func check(_ ok: Bool, _ what: String) {
        print(ok ? "ok   \(what)" : "FAIL \(what)")
        if !ok { failed += 1 }
    }

    // the fields as GitHub reads them: percent-decoded, + as a space
    static func fields(_ url: URL) -> [String: String] {
        var result: [String: String] = [:]
        for pair in (url.query ?? "").split(separator: "&") {
            let parts = pair.split(separator: "=", maxSplits: 1).map {
                $0.replacingOccurrences(of: "+", with: " ").removingPercentEncoding ?? "?"
            }
            result[parts[0]] = parts.count > 1 ? parts[1] : ""
        }
        return result
    }

    static func main() {
        let mac = "Apple M1 (MacBookAir10,1), 16 GB, macOS 27.0.1"
        let steam = Report.game(name: "Tom Clancy's Rainbow Six® & C++ #1 Größe", steamID: "359550",
                                message: "Tom Clancy's Rainbow Six® & C++ #1 Größe stopped right away (exit code 5).",
                                version: "0.5.0", mac: mac)
        let f = fields(steam)
        check(steam.absoluteString.hasPrefix("https://github.com/zienert/freequay/issues/new?template=game-report.yml&"), "game report form")
        check(f["title"] == "Game: Tom Clancy's Rainbow Six® & C++ #1 Größe", "title with ' ® & + # and umlauts")
        check(f["game"] == "Tom Clancy's Rainbow Six® & C++ #1 Größe, Steam app 359550", "game with its Steam app ID")
        check(f["mac"] == mac && f["version"] == "0.5.0", "Mac and version")
        check(f["log"] == "Tom Clancy's Rainbow Six® & C++ #1 Größe stopped right away (exit code 5).", "message in the log field")
        check(Set(f.keys) == ["template", "title", "game", "mac", "version", "log"], "only text fields, no choices")
        check(!(steam.query ?? "").contains("+"), "no + left in the address")

        let program = fields(Report.game(name: "Notepad++", steamID: nil, message: nil, version: "0.5.0", mac: mac))
        check(program["game"] == "Notepad++" && program["log"] == nil, "a program without Steam app ID or message")

        let empty = fields(Report.game(name: nil, steamID: nil, message: nil, version: "0.5.0", mac: mac))
        check(empty["title"] == nil && empty["game"] == nil && empty["template"] == "game-report.yml", "empty game report")

        let bug = Report.bug(version: "0.5.0", mac: mac)
        check(bug.absoluteString.hasPrefix("https://github.com/zienert/freequay/issues/new?template=bug.yml&")
              && fields(bug) == ["template": "bug.yml", "version": "0.5.0", "mac": mac], "bug report")

        check(Report.scrub("freequay: cannot read /Users/anna/Downloads/setup.exe", home: "/Users/anna", user: "anna")
              == "freequay: cannot read ~/Downloads/setup.exe", "home folder as ~")
        check(Report.scrub("wine: could not load L\"C:\\users\\Anna\\Desktop\\a.exe\"", home: "/Users/anna", user: "anna")
              == "wine: could not load L\"C:\\users\\you\\Desktop\\a.exe\"", "profile in the bottle without the user name")
        let long = Report.scrub(String(repeating: "x", count: 2000), home: "/Users/anna", user: "anna")
        check(long.count == 500 && long.hasSuffix("…"), "message cut to 500 characters")
        let longest = Report.game(name: String(repeating: "N", count: 200), steamID: "1", message: String(repeating: "é", count: 500),
                                  version: "0.5.0", mac: mac)
        check(longest.absoluteString.count < 8000, "address with the longest message stays under 8000 characters (\(longest.absoluteString.count))")

        print("this Mac: \(Report.mac)")
        check(Report.mac.range(of: #"^.+, [0-9]+ GB, macOS [0-9]+\.[0-9]+(\.[0-9]+)?$"#, options: .regularExpression) != nil,
              "this Mac as chip, memory and macOS")

        print(failed == 0 ? "all checks passed" : "\(failed) checks failed")
        exit(failed == 0 ? 0 : 1)
    }
}
