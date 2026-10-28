// Report on GitHub: the game report and the bug report of the repository (.github/ISSUE_TEMPLATE),
// filled in through the address of the form. GitHub fills only text fields this way, by their id;
// the choices stay for the user. Nothing leaves the Mac until the user submits the form on GitHub.
import Foundation

enum Report {
    static let forms = "https://github.com/zienert/freequay/issues/new"

    // A game report. name is the program as the list shows it, steamID its Steam app ID, message what
    // the window said after the start; none of them for an empty report.
    static func game(name: String?, steamID: String?, message: String?, version: String, mac: String) -> URL {
        var fields = [("template", "game-report.yml")]
        if let name {
            fields += [("title", "Game: \(name)"), ("game", steamID.map { "\(name), Steam app \($0)" } ?? name)]
        }
        fields += [("mac", mac), ("version", version)]
        if let message { fields.append(("log", message)) }
        return address(fields)
    }

    static func bug(version: String, mac: String) -> URL {
        address([("template", "bug.yml"), ("version", version), ("mac", mac)])
    }

    // Every character but letters, digits and -._~ is encoded: GitHub reads + in an address as a space.
    private static let plain = CharacterSet(charactersIn: "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~")

    private static func address(_ fields: [(String, String)]) -> URL {
        URL(string: forms + "?" + fields.map { "\($0.0)=\($0.1.addingPercentEncoding(withAllowedCharacters: plain) ?? "")" }
            .joined(separator: "&"))!
    }

    // A message names files by their path, on the Mac and in the bottle, where the user's profile is
    // C:\users\<name>; the user name stays out of the report. At most 500 characters.
    static func scrub(_ message: String, home: String = NSHomeDirectory(), user: String = NSUserName()) -> String {
        var text = message.replacingOccurrences(of: home, with: "~")
        if !user.isEmpty {
            text = text.replacingOccurrences(of: "\\users\\" + user, with: "\\users\\you", options: .caseInsensitive)
        }
        return text.count > 500 ? String(text.prefix(499)) + "…" : text
    }

    // chip, model, memory and macOS, as the forms ask for them: "Apple M1 (MacBookAir10,1), 16 GB, macOS 27.0.1"
    static let mac: String = {
        let chip = sysctl("machdep.cpu.brand_string") ?? "Apple silicon"
        let model = sysctl("hw.model").map { " (\($0))" } ?? ""
        let os = ProcessInfo.processInfo.operatingSystemVersion
        let macOS = "macOS \(os.majorVersion).\(os.minorVersion)" + (os.patchVersion > 0 ? ".\(os.patchVersion)" : "")
        return "\(chip)\(model), \(ProcessInfo.processInfo.physicalMemory >> 30) GB, \(macOS)"
    }()

    private static func sysctl(_ name: String) -> String? {
        var size = 0
        guard sysctlbyname(name, nil, &size, nil, 0) == 0, size > 1 else { return nil }
        var value = [CChar](repeating: 0, count: size)
        guard sysctlbyname(name, &value, &size, nil, 0) == 0 else { return nil }
        return String(cString: value)
    }
}
