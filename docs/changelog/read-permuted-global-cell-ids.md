# Fix reading global ids of permuted cells

The legacy VTK reader sometimes has to permute cell data because some VTK
cells are not directly supported in VTK-m. (For example, triangle strips
are not supported. They have to be converted to triangles.)

The global and petigree identifiers were not properly getting permuted.
This is now fixed.
