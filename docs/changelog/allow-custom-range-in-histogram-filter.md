#Allow histogram filter to take custom types

By passing TypeList and StorageList type into FieldRangeGlobalCompute,
upstream users(VTK) can pass custom types into the histogram filter.
