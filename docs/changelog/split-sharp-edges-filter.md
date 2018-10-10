Add a split sharp edge filter

It's a filter that splits sharp manifold edges where the feature angle
between the adjacent surfaces are larger than the threshold value.
When an edge is split, it would add a new point to the coordinates
and update the connectivity of an adjacent surface.
Ex. There are two adjacent triangles(0,1,2) and (2,1,3). Edge (1,2) needs
to be split. Two new points 4(duplication of point 1) an 5(duplication of point 2)
would be added and the later triangle's connectivity would be changed
to (5,4,3).
By default, all old point's fields would be copied to the new point.
Use with caution.
