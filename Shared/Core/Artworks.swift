import Foundation

/// A work for the placards. Drawn from the catalogues `data/artworks.json` (paintings)
/// and `data/sculptures.json`, plus `assets/collection.json` for the works of the other wings
/// (Chinese Wing, Hall of Light photographs, The Starry Night), all bundled as-is.
struct Artwork: Decodable, Identifiable, Hashable {
    let id: String
    let artist: String
    let title: String
    let originalTitle: String?
    let year: String
    let medium: String?
    let collection: String
    let city: String?
    let notes: String?
}

enum Catalogue {
    private struct Paintings: Decodable { let artworks: [Artwork] }
    private struct Sculptures: Decodable { let sculptures: [Artwork] }
    private struct Extra: Decodable { let works: [Artwork] }

    private static func load<T: Decodable>(_ name: String, _ type: T.Type) -> T? {
        guard let url = MuseumResources.url(name, "json"),
              let data = try? Data(contentsOf: url) else { return nil }
        do { return try JSONDecoder().decode(type, from: data) } catch {
            print("Catalogue: could not read \(name).json: \(error)")
            return nil
        }
    }

    static let all: [String: Artwork] = {
        var works: [Artwork] = []
        works += load("artworks", Paintings.self)?.artworks ?? []
        works += load("sculptures", Sculptures.self)?.sculptures ?? []
        works += load("collection", Extra.self)?.works ?? []
        return Dictionary(works.map { ($0.id, $0) }, uniquingKeysWith: { a, _ in a })
    }()

    static func artwork(_ id: String) -> Artwork? { all[id] }
}
