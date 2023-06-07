# New Isosurface Uncertainty Visualization Analysis filter

This new filter, designed for uncertainty analysis of the marching cubes algorithm for isosurface visualization, computes three uncertainty metrics, namely, level-crossing probability, topology case count, and entropy-based uncertainty. This filter requires a 3D cell-structured dataset with ensemble minimum and ensemble maximum values denoting uncertain data range at each grid vertex.

This uncertainty analysis filter analyzes the uncertainty in the marching cubes topology cases. The output dataset consists of 3D point fields corresponding to the level-crossing probability, topology case count, and entropy-based uncertainty. 

This VTK-m implementation is based on the algorithm presented in the paper "FunMC2: A Filter for Uncertainty Visualization of Marching Cubes on Multi-Core Devices" by Wang, Z., Athawale, T. M., Moreland, K., Chen, J., Johnson, C. R., & Pugmire, D. 
