# Implemented PNG/PPM image Readers/Writers

The original implementation of writing image data was only performed as a 
proxy through the Canvas rendering class. In order to implement true support
for image-based regression testing, this interface needed to be expanded upon
to support reading/writing arbitrary image data and storing it in a `vtkm::DataSet`.
Using the new `vtkm::io::PNGReader` and `vtkm::io::PPMReader` it is possible
to read data from files and Cavases directly and store them as a point field
in a 2D uniform `vtkm::DataSet`

```cpp
auto reader = vtkm::io::PNGReader();
auto imageDataSet = reader.ReadFromFile("read_image.png");
```

Similarly, the new `vtkm::io::PNGWriter` and `vtkm::io::PPMWriter` make it possible
to write out a 2D uniform `vtkm::DataSet` directly to a file.  

```cpp
auto writer = vtkm::io::PNGWriter();
writer.WriteToFile("write_image.png", imageDataSet);
```

If canvas data is to be written out, the reader provides a method for converting
a canvas's data to a `vtkm::DataSet`.

```cpp
auto reader = vtkm::io::PNGReader();
auto dataSet = reader.CreateImageDataSet(canvas);
auto writer = vtkm::io::PNGWriter();
writer.WriteToFile("output.png", dataSet);
```
