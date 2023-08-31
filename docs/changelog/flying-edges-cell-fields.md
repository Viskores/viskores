# Fix interpolation of cell fields with flying edges

The flying edges algorithm (used when contouring uniform structured cell
sets) was not interpolating cell fields correctly. There was an indexing
issue where a shortcut in the stepping was not incrementing the cell index.
