# Support writing binary files to legacy VTK files

The legacy VTK file writer writes out in ASCII. This is helpful when a
human is trying to read the file. However, if you have more than a
trivial amount of data, the file can get impractically large. To get
around this, `VTKDataSetWriter` now has a flag that allows you to write
the data in binary format.
