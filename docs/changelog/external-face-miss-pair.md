# Fixed error in checking paired faces in ExternalFaces filter

The `ExternalFaces` filter uses hash codes to find duplicate (i.e.
internal) faces. The issue with hash codes is that you have to deal with
unique entries that have identical hashes. The worklet to count how many
unique, unmatched faces were associated with each hash code was correct.
However, the code to then grab the ith unique face in a hash was wrong.
This has been fixed.
