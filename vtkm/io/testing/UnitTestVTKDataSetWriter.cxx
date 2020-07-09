//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

#define WRITE_FILE(MakeTestDataMethod)                                                             \
  TestVTKWriteTestData(#MakeTestDataMethod, tds.MakeTestDataMethod())

struct CheckSameField
{
  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& originalArray,
                  const vtkm::cont::Field& fileField) const
  {
    vtkm::cont::ArrayHandle<T> fileArray;
    fileField.GetData().CopyTo(fileArray);
    VTKM_TEST_ASSERT(test_equal_portals(originalArray.ReadPortal(), fileArray.ReadPortal()));
  }
};

struct CheckSameCoordinateSystem
{
  template <typename T>
  void operator()(const vtkm::cont::ArrayHandle<T>& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }

  void operator()(const vtkm::cont::ArrayHandleUniformPointCoordinates& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    VTKM_TEST_ASSERT(fileCoords.GetData().IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>());
    vtkm::cont::ArrayHandleUniformPointCoordinates fileArray =
      fileCoords.GetData().Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
    auto originalPortal = originalArray.ReadPortal();
    auto filePortal = fileArray.ReadPortal();
    VTKM_TEST_ASSERT(test_equal(originalPortal.GetOrigin(), filePortal.GetOrigin()));
    VTKM_TEST_ASSERT(test_equal(originalPortal.GetSpacing(), filePortal.GetSpacing()));
    VTKM_TEST_ASSERT(test_equal(originalPortal.GetRange3(), filePortal.GetRange3()));
  }

  using ArrayHandleRectilinearCoords =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;
  void operator()(const ArrayHandleRectilinearCoords& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    VTKM_TEST_ASSERT(fileCoords.GetData().IsType<ArrayHandleRectilinearCoords>());
    ArrayHandleRectilinearCoords fileArray =
      fileCoords.GetData().Cast<ArrayHandleRectilinearCoords>();
    auto originalPortal = originalArray.ReadPortal();
    auto filePortal = fileArray.ReadPortal();
    VTKM_TEST_ASSERT(
      test_equal_portals(originalPortal.GetFirstPortal(), filePortal.GetFirstPortal()));
    VTKM_TEST_ASSERT(
      test_equal_portals(originalPortal.GetSecondPortal(), filePortal.GetSecondPortal()));
    VTKM_TEST_ASSERT(
      test_equal_portals(originalPortal.GetThirdPortal(), filePortal.GetThirdPortal()));
  }

  void operator()(const vtkm::cont::ArrayHandleVirtualCoordinates& originalArray,
                  const vtkm::cont::CoordinateSystem& fileCoords) const
  {
    if (originalArray.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      (*this)(originalArray.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>(), fileCoords);
    }
    else if (originalArray.IsType<ArrayHandleRectilinearCoords>())
    {
      (*this)(originalArray.Cast<ArrayHandleRectilinearCoords>(), fileCoords);
    }
    else
    {
      CheckSameField{}(originalArray, fileCoords);
    }
  }
};

void CheckWrittenReadData(const vtkm::cont::DataSet& originalData,
                          const vtkm::cont::DataSet& fileData)
{
  VTKM_TEST_ASSERT(originalData.GetNumberOfPoints() == fileData.GetNumberOfPoints());
  VTKM_TEST_ASSERT(originalData.GetNumberOfCells() == fileData.GetNumberOfCells());

  for (vtkm::IdComponent fieldId = 0; fieldId < originalData.GetNumberOfFields(); ++fieldId)
  {
    vtkm::cont::Field originalField = originalData.GetField(fieldId);
    VTKM_TEST_ASSERT(fileData.HasField(originalField.GetName(), originalField.GetAssociation()));
    vtkm::cont::Field fileField =
      fileData.GetField(originalField.GetName(), originalField.GetAssociation());
    vtkm::cont::CastAndCall(originalField, CheckSameField{}, fileField);
  }

  VTKM_TEST_ASSERT(fileData.GetNumberOfCoordinateSystems() > 0);
  CheckSameCoordinateSystem{}(originalData.GetCoordinateSystem().GetData(),
                              fileData.GetCoordinateSystem());
}

void TestVTKWriteTestData(const std::string& methodName, const vtkm::cont::DataSet& data)
{
  std::cout << "Writing " << methodName << std::endl;
  vtkm::io::VTKDataSetWriter writer(methodName + ".vtk");
  writer.WriteDataSet(data);

  // Read back and check.
  vtkm::io::VTKDataSetReader reader(methodName + ".vtk");
  CheckWrittenReadData(data, reader.ReadDataSet());
}

void TestVTKExplicitWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make1DExplicitDataSet0);

  WRITE_FILE(Make2DExplicitDataSet0);

  WRITE_FILE(Make3DExplicitDataSet0);
  WRITE_FILE(Make3DExplicitDataSet1);
  WRITE_FILE(Make3DExplicitDataSet2);
  WRITE_FILE(Make3DExplicitDataSet3);
  WRITE_FILE(Make3DExplicitDataSet4);
  WRITE_FILE(Make3DExplicitDataSet5);
  WRITE_FILE(Make3DExplicitDataSet6);
  WRITE_FILE(Make3DExplicitDataSet7);
  WRITE_FILE(Make3DExplicitDataSet8);
  WRITE_FILE(Make3DExplicitDataSetZoo);
  WRITE_FILE(Make3DExplicitDataSetPolygonal);
  WRITE_FILE(Make3DExplicitDataSetCowNose);

  std::cout << "Force writer to output an explicit grid as points" << std::endl;
  vtkm::io::VTKDataSetWriter writer("Make3DExplicitDataSet0-no-grid.vtk");
  writer.WriteDataSet(tds.Make3DExplicitDataSet0(), true);
}

void TestVTKUniformWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make1DUniformDataSet0);
  WRITE_FILE(Make1DUniformDataSet1);
  WRITE_FILE(Make1DUniformDataSet2);

  WRITE_FILE(Make2DUniformDataSet0);
  WRITE_FILE(Make2DUniformDataSet1);
  WRITE_FILE(Make2DUniformDataSet2);

  WRITE_FILE(Make3DUniformDataSet0);
  WRITE_FILE(Make3DUniformDataSet1);
  // WRITE_FILE(Make3DUniformDataSet2); Skip this one. It's really big.
  WRITE_FILE(Make3DUniformDataSet3);

  WRITE_FILE(Make3DRegularDataSet0);
  WRITE_FILE(Make3DRegularDataSet1);

  std::cout << "Force writer to output a uniform grid as points" << std::endl;
  vtkm::io::VTKDataSetWriter writer("Make3DUniformDataSet0-no-grid.vtk");
  writer.WriteDataSet(tds.Make3DUniformDataSet0(), true);
}

void TestVTKRectilinearWrite()
{
  vtkm::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make2DRectilinearDataSet0);

  WRITE_FILE(Make3DRectilinearDataSet0);

  std::cout << "Force writer to output a rectilinear grid as points" << std::endl;
  vtkm::io::VTKDataSetWriter writer("Make3DRectilinearDataSet0-no-grid.vtk");
  writer.WriteDataSet(tds.Make3DRectilinearDataSet0(), true);
}

void TestVTKCompoundWrite()
{
  double s_min = 0.00001;
  double s_max = 1.0;
  double t_min = -2.0;
  double t_max = 2.0;
  int s_samples = 16;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dims(s_samples, s_samples);
  vtkm::Vec2f_64 origin(t_min, s_min);
  vtkm::Float64 ds = (s_max - s_min) / vtkm::Float64(dims[0] - 1);
  vtkm::Float64 dt = (t_max - t_min) / vtkm::Float64(dims[1] - 1);
  vtkm::Vec2f_64 spacing(dt, ds);
  vtkm::cont::DataSet dataSet = dsb.Create(dims, origin, spacing);
  size_t nVerts = static_cast<size_t>(s_samples * s_samples);
  std::vector<vtkm::Vec2f_64> points(nVerts);

  size_t idx = 0;
  for (vtkm::Id y = 0; y < dims[0]; ++y)
  {
    for (vtkm::Id x = 0; x < dims[1]; ++x)
    {
      double s = s_min + static_cast<vtkm::Float64>(y) * ds;
      double t = t_min + static_cast<vtkm::Float64>(x) * dt;
      // This function is not meaningful:
      auto z = std::exp(std::complex<double>(s, t));
      points[idx] = { std::sqrt(std::norm(z)), std::arg(z) };
      idx++;
    }
  }

  dataSet.AddPointField("z", points.data(), static_cast<vtkm::Id>(points.size()));
  vtkm::io::VTKDataSetWriter writer("chirp.vtk");
  writer.WriteDataSet(dataSet);
  std::remove("chirp.vtk");
}

void TestVTKWrite()
{
  TestVTKExplicitWrite();
  TestVTKUniformWrite();
  TestVTKRectilinearWrite();
  TestVTKCompoundWrite();
}

} //Anonymous namespace

int UnitTestVTKDataSetWriter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVTKWrite, argc, argv);
}
