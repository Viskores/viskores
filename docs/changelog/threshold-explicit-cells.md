# Copy Threshold output to a CellSetExplicit

Perhaps a better title for this change would be "Make the Threshold filter
not totally useless."
    
A long standing issue with the Threshold filter is that its output CellSet
was stored in a CellSetPermutation. This made Threshold hyper- efficient
because it required hardly any data movement to implement. However, the
problem was that any other unit that had to use the CellSet failed. To have
VTK-m handle that output correctly in other filters and writers, they all
would have to check for the existance of CellSetPermutation. And
CellSetPermutation is templated on the CellSet type it is permuting, so all
units would have to compile special cases for all these combinations. This
is not likely to be feasible in any static solution.
    
The simple solution, implemented here, is to deep copy the cells to a
CellSetExplicit, which is a known type that is already used everywhere in
VTK-m. The solution is a bit disappointing since it requires more memory
and time to build. But it is on par with solutions in other libraries (like
VTK). And it really does not matter how efficient the old solution was if
it was useless.
