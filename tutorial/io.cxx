//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// Example 1: very simple Viskores program.
// Read data set, write it out.
//
#include <viskores/cont/Initialize.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);

  const char* input = "data/kitchen.vtk";
  viskores::io::VTKDataSetReader reader(input);
  viskores::cont::DataSet ds = reader.ReadDataSet();
  viskores::io::VTKDataSetWriter writer("out_io.vtk");
  writer.WriteDataSet(ds);

  return 0;
}
