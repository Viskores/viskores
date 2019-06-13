//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/ZFPCompressor1D.h>
#include <vtkm/filter/ZFPCompressor2D.h>
#include <vtkm/filter/ZFPCompressor3D.h>
#include <vtkm/filter/ZFPDecompressor1D.h>
#include <vtkm/filter/ZFPDecompressor2D.h>
#include <vtkm/filter/ZFPDecompressor3D.h>

namespace vtkm_ut_zfp_filter
{

void TestZFP1DFilter(vtkm::Float64 rate)
{


  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make1DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float64> field =
    dynField.Cast<vtkm::cont::ArrayHandle<vtkm::Float64>>();
  auto oport = field.GetPortalControl();

  vtkm::filter::ZFPCompressor1D compressor;
  vtkm::filter::ZFPDecompressor1D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);



  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();
  ;
  field = dynField.Cast<vtkm::cont::ArrayHandle<vtkm::Float64>>();
  auto port = field.GetPortalControl();

  for (int i = 0; i < field.GetNumberOfValues(); i++)
  {
    std::cout << oport.Get(i) << " " << port.Get(i) << " " << oport.Get(i) - port.Get(i)
              << std::endl;
    ;
  }
}

void TestZFP2DFilter(vtkm::Float64 rate)
{


  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make2DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();
  ;
  vtkm::cont::ArrayHandle<vtkm::Float64> field =
    dynField.Cast<vtkm::cont::ArrayHandle<vtkm::Float64>>();
  auto oport = field.GetPortalControl();


  vtkm::filter::ZFPCompressor2D compressor;
  vtkm::filter::ZFPDecompressor2D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);



  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();
  ;
  field = dynField.Cast<vtkm::cont::ArrayHandle<vtkm::Float64>>();
  auto port = field.GetPortalControl();

  for (int i = 0; i < dynField.GetNumberOfValues(); i++)
  {
    std::cout << oport.Get(i) << " " << port.Get(i) << " " << oport.Get(i) - port.Get(i)
              << std::endl;
    ;
  }
}

void TestZFP3DFilter(vtkm::Float64 rate)
{


  const vtkm::Id3 dims(4, 4, 4);
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();
  vtkm::cont::ArrayHandle<vtkm::Float64> field =
    dynField.Cast<vtkm::cont::ArrayHandle<vtkm::Float64>>();
  auto oport = field.GetPortalControl();


  vtkm::filter::ZFPCompressor3D compressor;
  vtkm::filter::ZFPDecompressor3D decompressor;

  compressor.SetActiveField("pointvar");
  compressor.SetRate(rate);
  auto compressed = compressor.Execute(dataset);



  decompressor.SetActiveField("compressed");
  decompressor.SetRate(rate);
  auto decompress = decompressor.Execute(compressed);
  dynField = decompress.GetField("decompressed").GetData();
  ;
  field = dynField.Cast<vtkm::cont::ArrayHandle<vtkm::Float64>>();
  auto port = field.GetPortalControl();

  for (int i = 0; i < dynField.GetNumberOfValues(); i++)
  {
    std::cout << oport.Get(i) << " " << port.Get(i) << " " << oport.Get(i) - port.Get(i)
              << std::endl;
    ;
  }
}

void TestZFPFilter()
{
  TestZFP1DFilter(4);
  TestZFP2DFilter(4);
  TestZFP2DFilter(4);
}
} // anonymous namespace

int UnitTestZFP(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(vtkm_ut_zfp_filter::TestZFPFilter, argc, argv);
}
