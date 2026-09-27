# Photometric files

The makers' own photometric files for the luminaires whose light only a measured distribution describes. Each is
kept here byte for byte as its maker issued it (`.gitattributes` stops any line-ending conversion), because the
make reads it: Light integrates its candela table for the luminaire's flux, and Studio imports it through the
engine's IES import as the light profile the luminaire is lit by. A file is added only with the owner's approval.

## iguzzini-362G_O25F.ies

| | |
|---|---|
| Maker | iGuzzini illuminazione S.p.A. (manufacturer and test laboratory) |
| Product | Sipario LV ø 73, DALI Powerline, Wall Washer, lens: 16.1 W, 999 lm, 3000 K, CRI 97 |
| Page | [iguzzini.com/en/362g](https://www.iguzzini.com/en/362g/), its Downloads tab |
| LUMCAT | `362G_O25F` |
| Test | `887A0000.RV1` |
| Issue date | 03/07/2025, as the file writes it |
| Format | IESNA LM-63-2002, Type C, 181 vertical angles 0° to 180° and 37 horizontal angles 0° to 180° (bilateral symmetry about the 0°–180° plane), absolute photometry at rated input (`[_ABSOLUTELUMENS] 999`) |
| Obtained | 2026-09-27, downloaded by the owner from the maker's page and approved for the museum |
| SHA-256 | `751b968c995187d1f7908825762741e4e0b4349e3865f412dd3978997da69b9e` |
| Used by | `track-wallwasher.3000k` (`Data/Light/Luminaires/track-wallwasher.3000k.json`): the washers of the Classical Hall and Chenghuai |

Its candela table integrates to 999 lm, the rated flux, with a peak of 1566.3 cd at 21° from the nadir in the 0°
plane: the lens throws two thirds of its light into the half of the 0° plane, which the museum turns towards the
washed wall (`Docs/Light/Luminaires.md`, "Photometric files").
