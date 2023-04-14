# Continuous Scatterplot filter

This new filter, designed for bi-variate analysis, computes the continuous scatter-plot of a 3D mesh for two given scalar point fields.

The continuous scatter-plot is an extension of the discrete scatter-plot for continuous bi-variate analysis. The output dataset consists of triangle-shaped cells, whose coordinates on the 2D plane represent respectively the values of both scalar fields. The point centered scalar field generated on the triangular mesh quantifies the density of values in the data domain.

This VTK-m implementation is based on the algorithm presented in the paper "Continuous Scatterplots" by S. Bachthaler and D. Weiskopf.
