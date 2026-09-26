# Kelmscott Chaucer page images: sources

## Source item

- **Book:** *The Works of Geoffrey Chaucer now newly imprinted*. Hammersmith: Kelmscott Press (William Morris), 1896. Edited by F. S. Ellis. Woodcuts after Sir Edward Burne-Jones, engraved by W. H. Hooper. Printed in black and red on handmade paper. Folio, 44 cm.
- **Copy:** Ingalls Library, Cleveland Museum of Art (bequest of Julia Morgan Marlatt, per the bookplate on the front pastedown).
- **Digitisation:** Internet Archive item `MorrisChaucer`, uploaded by the Cleveland Museum of Art (Digitization@clevelandart.org), 25 February 2020. Scanned on an i2S Suprascan Quartz A1 at 600 ppi.
  - Item page: https://archive.org/details/MorrisChaucer
  - ARK: ark:/13960/t41s5bj3s
  - Page images: `https://archive.org/download/MorrisChaucer/page/n<N>.jpg` (full-size JPEG made from the `220953-MorrisChaucer_reduced_jp2.tar` JP2 set).
- **Licence:** The book is in the public domain (published 1896; Morris d. 1896, Burne-Jones d. 1898, Hooper d. 1912, Ellis d. 1901). The scan carries the **Creative Commons Public Domain Mark 1.0** (`licenseurl: http://creativecommons.org/publicdomain/mark/1.0/`) on the Internet Archive item. No other terms are attached.

Other copies checked but not used:
- Boston Public Library copy, IA `worksofgeoffreyc00chau_0`: a clean, complete scan, but only about 2112 x 3000 px per page and colder (greyer) paper. The pages sit on a black background, so they would need cropping.
- Birmingham Museums Trust 1934P675 (the Cobden-Sanderson pigskin binding), CC0. These are photographs of selected illustrated openings (about 10300 x 7750 px per spread) in a cradle with page clips. They are not consecutive pages, so they are not used here but could serve as reference for the bound book.

## Leaves used (reading order)

The Cleveland copy has a tissue guard between the title page and page 1. That is scan leaves n14 and n15, and it is left out, so that `page_000` + `page_001` form the real first opening.

| File | IA leaf | What |
|---|---|---|
| page_000.jpg | n13 | Woodcut title page, "the works of Geoffrey Chaucer now newly imprinted" (verso, left) |
| page_001.jpg | n16 | Printed page 1: Burne-Jones woodcut of Chaucer in the garden, "Here begynneth the Tales of Caunterbury and first the Prologue thereof", "Whan that Aprille..." (recto, right) |
| page_002.jpg ... page_039.jpg | n17 ... n54 | Printed pages 2 to 39, consecutive (page_NNN = printed page NNN). Even numbers are versos, odd numbers are rectos. |

Openings: 000|001 (title + Prologue), 002|003, 004|005, ... 038|039. Page 9 (page_009) opens The Knightes Tale with a woodcut. Several later openings carry full woodcut borders and pictures (pages 15, 22–24, 30–31, 37).

Note: in the Kelmscott Chaucer the left page of the first opening is the full-page woodcut **title**, not a portrait frontispiece. The Burne-Jones picture of Chaucer is at the head of page 1 on the right.

## Processing

- The IA derivatives are already cropped to the paper edge (no cradle, background or fingers are visible; all four edges are paper). 0.3% was trimmed on each side to stay just inside the edge.
- Skew was measured with a projection profile on every page. It was 0 ° (±0.05 °) on all pages except leaf n36, which measured about 0.25 ° and looked straight on inspection. No rotation was applied.
- Resized with Lanczos to a height of 2048 px. Every page is **1337 x 2048 px**.
- Colour was normalised with the same procedure on every page. The paper colour (85–97th percentile of lightness) and the black-ink colour (darkest 2% of neutral pixels) are measured per page, then each channel is mapped linearly so that paper → warm cream sRGB (233, 218, 192) and ink → warm black (30, 27, 26). Red ink is kept as it is. The bluish-grey show-through from the verso is desaturated towards the cream/black ramp, which removes the blue cast. The paper is not pushed to white.
- JPEG quality 92, 4:4:4 chroma.

## Aspect ratio

- Scan leaf: 6461 x 9900 px at 600 ppi = 27.4 x 41.9 cm, aspect (w/h) **0.653**. The output pages are 1337/2048 = **0.653**.
- The nominal leaf is about 29 x 42.5 cm (0.682). The scan is roughly 1.5 cm narrower. The likely reason is that part of the inner margin is hidden in the gutter of the tightly bound copy. The heights agree (41.9 vs about 42–42.5 cm). For the lectern model, either use 0.653 (what the image shows) or scale the image width about 4% to the nominal 0.682 and accept a slightly wider inner margin.
