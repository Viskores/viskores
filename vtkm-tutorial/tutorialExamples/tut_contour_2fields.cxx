// Example 3: do a contour (but only evaluate two fields), write it out.
//
#include <vtkm/cont/Initialize.h>
#include <vtkm/filter/Contour.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::io::VTKDataSetReader reader("data/kitchen.vtk");
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();

  vtkm::filter::Contour contour;
  contour.SetActiveField("c1");
  contour.SetFieldsToPass({ "c1", "ke" });
  contour.SetNumberOfIsoValues(3);
  contour.SetIsoValue(0, 0.05);
  contour.SetIsoValue(1, 0.10);
  contour.SetIsoValue(2, 0.15);

  vtkm::cont::DataSet ds_from_mc = contour.Execute(ds_from_file);
  vtkm::io::VTKDataSetWriter writer("out_mc_2fields.vtk");
  writer.WriteDataSet(ds_from_mc);

  return 0;
}
