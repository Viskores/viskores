# Move .cxx from DEVICE_SOURCES to SORUCES in vktm io library

It used to be that every .cxx file which uses ArrayHandle needs to be compiled
by the device compiler. A recent change had removed this restriction.
One exception is that user of ArrayCopy still requires device compiler.
Since most .cxx files in vtkm/io do not use ArrayCopy, they are moved
to SOURCES and are compiled by host compiler.
