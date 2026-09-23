import Foundation

/// Finds the museum's data files (paintings, scans, stars, catalogues). The app finds them in its
/// bundle; a tool that builds the museum outside the app (the USD exporter in `tools/usd-export/`)
/// lists the repository's asset folders in `folders` instead.
enum MuseumResources {
    static var folders: [URL] = []

    static func url(_ name: String, _ ext: String, subdirectory: String? = nil) -> URL? {
        if let subdirectory, let url = Bundle.main.url(forResource: name, withExtension: ext, subdirectory: subdirectory) {
            return url
        }
        if let url = Bundle.main.url(forResource: name, withExtension: ext) { return url }
        for folder in folders {
            let url = folder.appendingPathComponent(name).appendingPathExtension(ext)
            if FileManager.default.fileExists(atPath: url.path) { return url }
        }
        return nil
    }
}
