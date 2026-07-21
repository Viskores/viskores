//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

#define WRITE_FILE(MakeTestDataMethod) \
  TestVTKWriteTestData(#MakeTestDataMethod, tds.MakeTestDataMethod())

struct ScopedOutputFile
{
  explicit ScopedOutputFile(std::string fileName)
    : FileName(fileName)
  {
  }

  ~ScopedOutputFile() { std::remove(this->FileName.c_str()); }

  const std::string& GetFileName() const { return this->FileName; }

private:
  std::string FileName;
};

std::string ReadFileContents(const std::string& fileName)
{
  std::ifstream input(fileName, std::ios::binary);
  VISKORES_TEST_ASSERT(static_cast<bool>(input), "Failed to open generated file: " + fileName);

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

struct CheckSameField
{
  template <typename T, typename S>
  void operator()(const viskores::cont::ArrayHandle<T, S>& originalArray,
                  const viskores::cont::Field& fileField) const
  {
    viskores::cont::ArrayHandle<T> fileArray;
    fileField.GetData().AsArrayHandle(fileArray);
    VISKORES_TEST_ASSERT(test_equal_portals(originalArray.ReadPortal(), fileArray.ReadPortal()));
  }
};

struct CheckSameCoordinateSystem
{
  template <typename T>
  void operator()(const viskores::cont::ArrayHandle<T>& originalArray,
                  const viskores::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }

  template <typename T>
  void operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagSOA>& originalArray,
    const viskores::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }

  template <typename T>
  void operator()(
    const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagSOAStride>& originalArray,
    const viskores::cont::CoordinateSystem& fileCoords) const
  {
    CheckSameField{}(originalArray, fileCoords);
  }

  void operator()(const viskores::cont::ArrayHandleUniformPointCoordinates& originalArray,
                  const viskores::cont::CoordinateSystem& fileCoords) const
  {
    VISKORES_TEST_ASSERT(
      fileCoords.GetData().IsType<viskores::cont::ArrayHandleUniformPointCoordinates>());
    viskores::cont::ArrayHandleUniformPointCoordinates fileArray =
      fileCoords.GetData().AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
    auto originalPortal = originalArray.ReadPortal();
    auto filePortal = fileArray.ReadPortal();
    VISKORES_TEST_ASSERT(test_equal(originalPortal.GetOrigin(), filePortal.GetOrigin()));
    VISKORES_TEST_ASSERT(test_equal(originalPortal.GetSpacing(), filePortal.GetSpacing()));
    VISKORES_TEST_ASSERT(test_equal(originalPortal.GetRange3(), filePortal.GetRange3()));
  }

  template <typename T>
  using ArrayHandleRectilinearCoords =
    viskores::cont::ArrayHandle<T,
                                typename viskores::cont::ArrayHandleCartesianProduct<
                                  viskores::cont::ArrayHandle<T>,
                                  viskores::cont::ArrayHandle<T>,
                                  viskores::cont::ArrayHandle<T>>::StorageTag>;
  template <typename T>
  void operator()(const ArrayHandleRectilinearCoords<T>& originalArray,
                  const viskores::cont::CoordinateSystem& fileCoords) const
  {
    VISKORES_TEST_ASSERT(fileCoords.GetData().IsType<ArrayHandleRectilinearCoords<T>>());
    ArrayHandleRectilinearCoords<T> fileArray =
      fileCoords.GetData().AsArrayHandle<ArrayHandleRectilinearCoords<T>>();
    auto originalPortal = originalArray.ReadPortal();
    auto filePortal = fileArray.ReadPortal();
    VISKORES_TEST_ASSERT(
      test_equal_portals(originalPortal.GetFirstPortal(), filePortal.GetFirstPortal()));
    VISKORES_TEST_ASSERT(
      test_equal_portals(originalPortal.GetSecondPortal(), filePortal.GetSecondPortal()));
    VISKORES_TEST_ASSERT(
      test_equal_portals(originalPortal.GetThirdPortal(), filePortal.GetThirdPortal()));
  }

#ifdef VISKORES_ADD_XGC_DEFAULT_TYPES
  // Just added to fix compilation errors when building with XGC types added to default types
  // An XGC data set wouldn't be directly written out to a VTK file, it should be converted
  // to an explicit grid first and then written out.
  template <typename T>
  void operator()(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagXGCCoordinates>&,
                  const viskores::cont::CoordinateSystem&) const
  {
    throw viskores::cont::ErrorBadType(
      "UnitTestVTKDataSetWriter::CheckSameCoordinateSystem() shouldn't"
      " be called on ArrayHandleXGCCoordinates");
  }
#endif
};

void CheckWrittenReadData(const viskores::cont::DataSet& originalData,
                          const viskores::cont::DataSet& fileData)
{
  VISKORES_TEST_ASSERT(originalData.GetNumberOfPoints() == fileData.GetNumberOfPoints());
  VISKORES_TEST_ASSERT(originalData.GetNumberOfCells() == fileData.GetNumberOfCells());

  for (viskores::IdComponent fieldId = 0; fieldId < originalData.GetNumberOfFields(); ++fieldId)
  {
    viskores::cont::Field originalField = originalData.GetField(fieldId);
    if (originalField.IsPointField() &&
        (originalField.GetName() == originalData.GetCoordinateSystemName()))
    {
      // Do not check the field that is the first coordinate system. It is likely to have
      // changed name because VTK does not name coordinate systems.
      continue;
    }
    VISKORES_TEST_ASSERT(
      fileData.HasField(originalField.GetName(), originalField.GetAssociation()));
    viskores::cont::Field fileField =
      fileData.GetField(originalField.GetName(), originalField.GetAssociation());
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(originalField.GetData(), fileField.GetData()));
  }

  VISKORES_TEST_ASSERT(fileData.GetNumberOfCoordinateSystems() > 0);
  viskores::cont::CastAndCall(originalData.GetCoordinateSystem().GetData(),
                              CheckSameCoordinateSystem{},
                              fileData.GetCoordinateSystem());
}

void TestVTKWriteTestData(const std::string& methodName, const viskores::cont::DataSet& data)
{
  std::cout << "Writing " << methodName << std::endl;
  viskores::io::VTKDataSetWriter writer(methodName + ".vtk");
  writer.WriteDataSet(data);

  // Read back and check.
  viskores::io::VTKDataSetReader reader(methodName + ".vtk");
  CheckWrittenReadData(data, reader.ReadDataSet());

  std::cout << "Writing " << methodName << " ascii" << std::endl;
  viskores::io::VTKDataSetWriter writerAscii(methodName + "-ascii.vtk");
  writerAscii.SetFileTypeToAscii();
  writerAscii.WriteDataSet(data);

  // Read back and check.
  viskores::io::VTKDataSetReader readerAscii(methodName + "-ascii.vtk");
  CheckWrittenReadData(data, readerAscii.ReadDataSet());

  std::cout << "Writing " << methodName << " binary" << std::endl;
  viskores::io::VTKDataSetWriter writerBinary(methodName + "-binary.vtk");
  writerBinary.SetFileTypeToBinary();
  writerBinary.WriteDataSet(data);

  // Read back and check.
  viskores::io::VTKDataSetReader readerBinary(methodName + "-binary.vtk");
  CheckWrittenReadData(data, readerBinary.ReadDataSet());
}

void TestVTKExplicitWrite()
{
  viskores::cont::testing::MakeTestDataSet tds;

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

  std::cout << "Set writer to output an explicit grid" << std::endl;
  viskores::io::VTKDataSetWriter writer("Make3DExplicitDataSet0.vtk");
  writer.WriteDataSet(tds.Make3DExplicitDataSet0());
}

void TestVTKUniformWrite()
{
  viskores::cont::testing::MakeTestDataSet tds;

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

  std::cout << "Set writer to output an uniform grid" << std::endl;
  viskores::io::VTKDataSetWriter writer("Make3DUniformDataSet0.vtk");
  writer.WriteDataSet(tds.Make3DUniformDataSet0());
}

void TestVTKRectilinearWrite()
{
  viskores::cont::testing::MakeTestDataSet tds;

  WRITE_FILE(Make2DRectilinearDataSet0);

  WRITE_FILE(Make3DRectilinearDataSet0);

  std::cout << "Set writer to output a rectilinear grid" << std::endl;
  viskores::io::VTKDataSetWriter writer("Make3DRectilinearDataSet0.vtk");
  writer.WriteDataSet(tds.Make3DRectilinearDataSet0());
}

void TestVTKCompoundWrite()
{
  double s_min = 0.00001;
  double s_max = 1.0;
  double t_min = -2.0;
  double t_max = 2.0;
  int s_samples = 16;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dims(s_samples, s_samples);
  viskores::Vec2f_64 origin(t_min, s_min);
  viskores::Float64 ds = (s_max - s_min) / viskores::Float64(dims[0] - 1);
  viskores::Float64 dt = (t_max - t_min) / viskores::Float64(dims[1] - 1);
  viskores::Vec2f_64 spacing(dt, ds);
  viskores::cont::DataSet dataSet = dsb.Create(dims, origin, spacing);
  size_t nVerts = static_cast<size_t>(s_samples * s_samples);
  std::vector<viskores::Vec2f_64> points(nVerts);

  size_t idx = 0;
  for (viskores::Id y = 0; y < dims[0]; ++y)
  {
    for (viskores::Id x = 0; x < dims[1]; ++x)
    {
      double s = s_min + static_cast<viskores::Float64>(y) * ds;
      double t = t_min + static_cast<viskores::Float64>(x) * dt;
      // This function is not meaningful:
      auto z = std::exp(std::complex<double>(s, t));
      points[idx] = { std::sqrt(std::norm(z)), std::arg(z) };
      idx++;
    }
  }

  dataSet.AddPointField("z", points.data(), static_cast<viskores::Id>(points.size()));
  viskores::io::VTKDataSetWriter writer("chirp.vtk");
  writer.WriteDataSet(dataSet);
  std::remove("chirp.vtk");
}

viskores::cont::DataSet MakeOddVecSizeDataSet()
{
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::cont::DataSet dataSet = dsb.Create({ 2, 2, 2 });

  viskores::cont::ArrayHandle<viskores::Vec<viskores::FloatDefault, 5>> vec5Array;
  vec5Array.Allocate(dataSet.GetNumberOfPoints());
  SetPortal(vec5Array.WritePortal());
  dataSet.AddPointField("vec5", vec5Array);

  viskores::cont::ArrayHandleSOA<viskores::Vec<viskores::FloatDefault, 13>> vec13Array;
  vec13Array.Allocate(dataSet.GetNumberOfPoints());
  SetPortal(vec13Array.WritePortal());
  dataSet.AddPointField("vec13", vec13Array);

  return dataSet;
}

void TestVTKOddVecSizes()
{
  TestVTKWriteTestData("OddVecSizes", MakeOddVecSizeDataSet());
}

void TestVTKHighComponentFieldsUseFieldData()
{
  auto dataSet = MakeOddVecSizeDataSet();
  ScopedOutputFile outputFile("OddVecSizes-field-data.vtk");

  viskores::io::VTKDataSetWriter writer(outputFile.GetFileName());
  writer.SetFileTypeToAscii();
  writer.WriteDataSet(dataSet);

  // Round-tripping through our reader is not enough here; inspect the raw file to ensure
  // high-component arrays are no longer emitted as invalid SCALARS records.
  const std::string contents = ReadFileContents(outputFile.GetFileName());
  VISKORES_TEST_ASSERT(contents.find("POINT_DATA 8\n") != std::string::npos,
                       "Missing POINT_DATA header for high-component point fields.");
  VISKORES_TEST_ASSERT(contents.find("FIELD FieldData 2\n") != std::string::npos,
                       "High-component point fields should be written as FIELD data.");
  VISKORES_TEST_ASSERT(contents.find("vec5 5 8 ") != std::string::npos,
                       "Missing FIELD entry for vec5.");
  VISKORES_TEST_ASSERT(contents.find("vec13 13 8 ") != std::string::npos,
                       "Missing FIELD entry for vec13.");
  VISKORES_TEST_ASSERT(contents.find("SCALARS vec5") == std::string::npos,
                       "vec5 should not be written as SCALARS.");
  VISKORES_TEST_ASSERT(contents.find("SCALARS vec13") == std::string::npos,
                       "vec13 should not be written as SCALARS.");

  viskores::io::VTKDataSetReader reader(outputFile.GetFileName());
  CheckWrittenReadData(dataSet, reader.ReadDataSet());
}

void TestVTKRejectsUnsupportedFieldType()
{
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::cont::DataSet dataSet = dsb.Create({ 2, 2, 2 });
  // Use a trivially copyable type that Viskores can safely store on all enabled backends,
  // but that the legacy VTK writer still does not recognize as a supported scalar base type.
  std::vector<viskores::Pair<viskores::Int32, viskores::Int32>> unsupported(
    static_cast<std::size_t>(dataSet.GetNumberOfPoints()), { 1, 2 });
  dataSet.AddPointField("unsupported_pair", unsupported);

  ScopedOutputFile outputFile("UnsupportedFieldType.vtk");
  viskores::io::VTKDataSetWriter writer(outputFile.GetFileName());

  try
  {
    writer.WriteDataSet(dataSet);
    VISKORES_TEST_FAIL("Writer should reject unsupported point fields.");
  }
  catch (const viskores::cont::ErrorBadValue&)
  {
    // Expected.
  }
}

void TestVTKRejectsMismatchedFieldSize()
{
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::cont::DataSet dataSet = dsb.Create({ 2, 2, 2 });
  std::vector<viskores::FloatDefault> badField(
    static_cast<std::size_t>(dataSet.GetNumberOfPoints() - 1), 0.0f);
  dataSet.AddPointField("bad_field", badField);

  ScopedOutputFile outputFile("MismatchedFieldSize.vtk");
  viskores::io::VTKDataSetWriter writer(outputFile.GetFileName());

  try
  {
    writer.WriteDataSet(dataSet);
    VISKORES_TEST_FAIL("Writer should reject point fields with mismatched tuple counts.");
  }
  catch (const viskores::cont::ErrorBadValue&)
  {
    // Expected.
  }
}

void TestVTKReportsOutputErrors()
{
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::cont::DataSet dataSet = dsb.Create({ 2, 2, 2 });

  viskores::io::VTKDataSetWriter writer(
    "vtk_writer_missing_dir_for_test/does_not_exist/UnitTestVTKDataSetWriter.vtk");

  try
  {
    writer.WriteDataSet(dataSet);
    VISKORES_TEST_FAIL(
      "Writer should report filesystem errors when the output path cannot be opened.");
  }
  catch (const viskores::io::ErrorIO&)
  {
    // Expected.
  }
}

void TestVTKWrite()
{
  TestVTKExplicitWrite();
  TestVTKUniformWrite();
  TestVTKRectilinearWrite();
  TestVTKCompoundWrite();
  TestVTKOddVecSizes();
  TestVTKHighComponentFieldsUseFieldData();
  TestVTKRejectsUnsupportedFieldType();
  TestVTKRejectsMismatchedFieldSize();
  TestVTKReportsOutputErrors();
}

} //Anonymous namespace

int UnitTestVTKDataSetWriter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestVTKWrite, argc, argv);
}
