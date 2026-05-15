# Validation report: SIM-SR100-EQLEN-GAP

- Dataset: `benchmark/data/generated/SIM-SR100-EQLEN-GAP`
- Mode: `affine`
- Checked pairs: 10000
- Pair ID sets match: True
- Length mismatches: 0
- Equal read/reference length mismatches: 0
- Metadata equal_length mismatches: 0
- Per-pair inserted/deleted base imbalance: 0
- Alphabet mismatches: 0
- Simulated Levenshtein count mismatches: 0
- Stored DP Levenshtein mismatches: 0
- Edit-script affine score mismatches: 0

## Equal-length mixed-edit checks

- All checked pairs are equal length: True
- All checked pairs have balanced inserted/deleted bases: True
- Pairs with substitutions: 7039 (0.704)
- Pairs with insertions: 6149 (0.615)
- Pairs with deletions: 6149 (0.615)
- Pairs with both insertion and deletion: 6149 (0.615)
- Pairs with all three edit types: 3676 (0.368)

- Gotoh DP vs simulated script affine differences: 2036

- Stored Gotoh affine mismatches: 0

Gotoh DP can be lower than the simulated script score when random edits create an alternate lower-cost alignment. Use the Gotoh DP score as the authoritative affine oracle when benchmark metadata is extended with that field.
