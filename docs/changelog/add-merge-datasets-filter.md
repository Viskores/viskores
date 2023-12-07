# Adding MergeDataSets filter

The MergeDataSets filter can accept partitioned data sets and output a merged data set. It assumes that all partitions have the same coordinate systems. If a field is missing in a specific partition, the user-specified invalid value will be adopted and filled into the corresponding places of the merged field array.
