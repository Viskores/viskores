//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ZFPCompressor.h>
#include <vtkm/worklet/ZFPDecompress.h>

#include <vtkm/worklet/ZFP1DCompressor.h>
#include <vtkm/worklet/ZFP1DDecompress.h>

#include <vtkm/worklet/ZFP2DCompressor.h>
#include <vtkm/worklet/ZFP2DDecompress.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/zfp/ZFPTools.h>

#include <algorithm>
#include <fstream>
#include <iostream>


template <typename T>
void writeArray(vtkm::cont::ArrayHandle<T>& field, std::string filename)
{
  auto val = vtkm::worklet::zfp::detail::GetVTKMPointer(field);
  std::ofstream output(filename, std::ios::binary | std::ios::out);
  output.write(reinterpret_cast<char*>(val), field.GetNumberOfValues() * 8);
  output.close();

  for (int i = 0; i < field.GetNumberOfValues(); i++)
  {
    std::cout << val[i] << " ";
  }
  std::cout << std::endl;
}

using Handle64 = vtkm::cont::ArrayHandle<vtkm::Float64>;
template <typename Scalar>
void Test1D(int rate)
{
  std::cout << "Testing ZFP 1d:" << std::endl;
  vtkm::Id dims = 256;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make1DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();

  vtkm::worklet::ZFP1DCompressor compressor;
  vtkm::worklet::ZFP1DDecompressor decompressor;

  if (vtkm::cont::IsType<Handle64>(dynField))
  {
    vtkm::cont::ArrayHandle<Scalar> handle;
    const vtkm::Id size = dynField.Cast<Handle64>().GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = dynField.Cast<Handle64>().GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (vtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }

    auto compressed = compressor.Compress(handle, rate, dims);
    //writeArray(compressed, "output.zfp");
    vtkm::cont::ArrayHandle<Scalar> decoded;
    decompressor.Decompress(compressed, decoded, rate, dims);
    auto oport = decoded.GetPortalConstControl();
    for (int i = 0; i < 4; i++)
    {
      std::cout << oport.Get(i) << " " << fPortal.Get(i) << " " << oport.Get(i) - fPortal.Get(i)
                << std::endl;
    }
  }
}
template <typename Scalar>
void Test2D(int rate)
{
  std::cout << "Testing ZFP 2d:" << std::endl;
  vtkm::Id2 dims(16, 16);
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make2DUniformDataSet2();
  auto dynField = dataset.GetField("pointvar").GetData();

  vtkm::worklet::ZFP2DCompressor compressor;
  vtkm::worklet::ZFP2DDecompressor decompressor;

  if (vtkm::cont::IsType<Handle64>(dynField))
  {
    vtkm::cont::ArrayHandle<Scalar> handle;
    const vtkm::Id size = dynField.Cast<Handle64>().GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = dynField.Cast<Handle64>().GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (vtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }

    auto compressed = compressor.Compress(handle, rate, dims);
    vtkm::cont::ArrayHandle<Scalar> decoded;
    decompressor.Decompress(compressed, decoded, rate, dims);
    auto oport = decoded.GetPortalConstControl();
    for (int i = 0; i < 4; i++)
    {
      std::cout << oport.Get(i) << " " << fPortal.Get(i) << " " << oport.Get(i) - fPortal.Get(i)
                << std::endl;
    }
  }
}
template <typename Scalar>
void Test3D(int rate)
{
  std::cout << "Testing ZFP 3d:" << std::endl;
  vtkm::Id3 dims(4, 4, 4);
  //vtkm::Id3 dims(4,4,7);
  //vtkm::Id3 dims(8,8,8);
  //vtkm::Id3 dims(256,256,256);
  //vtkm::Id3 dims(128,128,128);
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataset = testDataSet.Make3DUniformDataSet3(dims);
  auto dynField = dataset.GetField("pointvar").GetData();

  vtkm::worklet::ZFPCompressor compressor;
  vtkm::worklet::ZFPDecompressor decompressor;

  if (vtkm::cont::IsType<Handle64>(dynField))
  {
    vtkm::cont::ArrayHandle<Scalar> handle;
    const vtkm::Id size = dynField.Cast<Handle64>().GetNumberOfValues();
    handle.Allocate(size);

    auto fPortal = dynField.Cast<Handle64>().GetPortalControl();
    auto hPortal = handle.GetPortalControl();
    for (vtkm::Id i = 0; i < size; ++i)
    {
      hPortal.Set(i, static_cast<Scalar>(fPortal.Get(i)));
    }

    auto compressed = compressor.Compress(handle, rate, dims);

    vtkm::cont::ArrayHandle<Scalar> decoded;
    decompressor.Decompress(compressed, decoded, rate, dims);
    auto oport = decoded.GetPortalConstControl();
    for (int i = 0; i < 4; i++)
    {
      std::cout << oport.Get(i) << " " << fPortal.Get(i) << " " << oport.Get(i) - fPortal.Get(i)
                << std::endl;
    }
  }
}

void TestZFP()
{
  Test3D<vtkm::Float64>(4);
  Test2D<vtkm::Float64>(4);
  Test1D<vtkm::Float64>(4);
  //Test3D<vtkm::Float32>(4);
  //Test3D<vtkm::Int64>(4);
  //Test3D<vtkm::Int32>(4);
}

int UnitTestZFPCompressor(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestZFP, argc, argv);
}
