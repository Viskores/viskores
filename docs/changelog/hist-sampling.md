
# Added a `HistSampling` filter

This filter assumes the field data are point clouds. It samples the field data according to its importance level. The importance level (sampling rate) is computed based on the histogram. The rarer values can provide more importance. More details can be found in the following paper. “In Situ Data-Driven Adaptive Sampling for Large-scale Simulation Data Summarization”, Ayan Biswas, Soumya Dutta, Jesus Pulido, and James Ahrens, In Situ Infrastructures for Enabling Extreme-scale Analysis and Visualization (ISAV 2018), co-located with Supercomputing 2018.
