// Example 1: very simple VTK-m program.
// Read data set, write it out.
//
#include <vtkm/cont/Initialize.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  const char* input = "data/kitchen.vtk";
  vtkm::io::VTKDataSetReader reader(input);
  vtkm::cont::DataSet ds = reader.ReadDataSet();
  vtkm::io::VTKDataSetWriter writer("out_io.vtk");
  writer.WriteDataSet(ds);

  return 0;
}
