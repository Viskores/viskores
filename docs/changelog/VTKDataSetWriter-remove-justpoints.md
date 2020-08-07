# Remove VTKDataSetWriter::WriteDataSet just_points parameter

In the method `VTKDataSetWriter::WriteDataSet`, `just_points` parameter has been
removed due to lack of usage. 

The purpose of `just_points` was to allow exporting only the points of a
DataSet without its cell data.
