## Shorten header filenames to fix Windows MAX_PATH install failures

Viskores header files were renamed to have shorter paths so that they can be
installed on Windows systems without Long Path support enabled. This fixes
failures seen when installing Viskores via conda on Windows instances where
the total installed path exceeded the 260-character MAX_PATH limit.

A CI check is now enforced to ensure no source file path exceeds 130
characters, leaving sufficient headroom for typical install prefixes.
