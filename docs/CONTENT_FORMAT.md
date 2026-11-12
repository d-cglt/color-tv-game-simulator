# Proposed content format

This is a project convention, not a historical file format. CTG1 IDs are stable for compatibility; the historical names, identities and console availability remain provisional.

## Encoding

A content file is exactly **5 bytes**, with no header fields beyond the following and no trailing data:

| Byte offset | Size | Meaning |
| --- | --- | --- |
| 0 | 4 bytes | ASCII magic `CTG1` (`43 54 47 31` in hexadecimal) |
| 4 | 1 byte | Unsigned content ID |

For example, `43 54 47 31 00` represents ID 0. The ID is one byte, so byte order is not applicable. The loader rejects files of any other length, an incorrect magic, or an ID outside 0–15.

## Provisional IDs

The former public B group is now Original and the former A group Variant. These project labels do not establish historical identity or primacy. IDs and gameplay remain unchanged, and the `6 + 15` assignment remains provisional; see [research notes](RESEARCH.md).

Generated filenames include, for example, `Tennis Variant - Singles (Color TV Game 15).ctg` and `Tennis Original - Singles (Color TV Game 6 + 15).ctg`. The `6 + 15` suffix remains tied to IDs 2, 3, 6, 7, 10 and 11, not to a historical verification. The remaining IDs 0–14 use `(Color TV Game 15)`; ID 14 is generated as `Shooting Game (Color TV Game 15).ctg`. ID 15 generates the single descriptor `Racing 112 (Color TV Game Racing 112).ctg`; it represents Racing 112 as a whole, not 112 separate files. This assignment does not by itself implement Racing gameplay.

| Decimal ID | Hex ID | Assignment |
| ---: | :---: | --- |
| 0 | `00` | Tennis Variant Singles |
| 1 | `01` | Tennis Variant Doubles |
| 2 | `02` | Tennis Original Singles |
| 3 | `03` | Tennis Original Doubles |
| 4 | `04` | Hockey Variant Singles |
| 5 | `05` | Hockey Variant Doubles |
| 6 | `06` | Hockey Original Singles |
| 7 | `07` | Hockey Original Doubles |
| 8 | `08` | Volleyball Variant Singles |
| 9 | `09` | Volleyball Variant Doubles |
| 10 | `0A` | Volleyball Original Singles |
| 11 | `0B` | Volleyball Original Doubles |
| 12 | `0C` | Ping-Pong Singles |
| 13 | `0D` | Ping-Pong Doubles |
| 14 | `0E` | Shooting Game |
| 15 | `0F` | Racing 112 |

Values `10` through `FF` are unassigned. The **Generate game files** core option writes these descriptors to `Nintendo Color TV-Game` under the frontend system directory. The directory must be available and writable; there is no fallback. Existing paths are skipped, not overwritten. Files in the former `system/ctggames` and save-directory locations are not copied or moved. Previously generated files retain their old names and encoded IDs; they are not renamed or removed, including files from the temporary inversion. Older A/B filenames from before and during the inversion can encode different IDs despite matching labels; inspect the encoded ID rather than relying on the filename. Existing bundled `slot-XX.ctg` files are unchanged; `slot-15.ctg` contains ID 15. A failed write can leave a partial file that must be removed manually before retrying. See the [roadmap](ROADMAP.md) for the research and implementation plan.
