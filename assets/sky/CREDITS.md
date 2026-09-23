# Sky data

`stars.bin` is built from the **Yale Bright Star Catalogue, 5th revised edition** (Hoffleit & Warren,
1991), VizieR catalogue V/50, https://cdsarc.cds.unistra.fr/viz-bin/cat/V/50 — every star to about
magnitude 6.5 (the naked-eye sky), J2000 positions. Catalogue data from the CDS/NASA ADC, freely
available for any use.

Format (little-endian): `MVS1`, uint32 count, then per star float32 right ascension (rad),
declination (rad), V magnitude, B−V colour index; sorted brightest first.
