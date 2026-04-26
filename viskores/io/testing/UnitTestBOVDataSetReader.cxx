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

#include <string>
#include <vector>

#include <viskores/cont/testing/Testing.h>
#include <viskores/io/BOVDataSetReader.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/io/FileUtils.h>
#include <viskores/io/internal/Endian.h>

#include <fstream>

namespace
{

inline viskores::cont::DataSet ReadBOVDataSet(const char* fname)
{
  viskores::cont::DataSet ds;
  viskores::io::BOVDataSetReader reader(fname);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  return ds;
}

std::string TestFileName(const std::string& filename)
{
  return "UnitTestBOVDataSetReader-" + filename;
}

std::string TestPath(const std::string& filename)
{
  return viskores::cont::testing::Testing::WriteDirPath(TestFileName(filename));
}

void WriteTextFile(const std::string& filename, const std::string& text)
{
  std::ofstream out(filename.c_str());
  VISKORES_TEST_ASSERT(out.good(), "Unable to open test text file");
  out << text;
  VISKORES_TEST_ASSERT(out.good(), "Unable to write test text file");
}

template <typename T>
void WriteBinaryFile(const std::string& filename,
                     const std::vector<T>& values,
                     bool fileLittleEndian = viskores::io::internal::IsLittleEndian(),
                     const std::vector<viskores::UInt8>& prefix = std::vector<viskores::UInt8>())
{
  std::ofstream out(filename.c_str(), std::ios::binary);
  VISKORES_TEST_ASSERT(out.good(), "Unable to open test binary file");

  for (viskores::UInt8 byte : prefix)
  {
    const char ch = static_cast<char>(byte);
    out.write(&ch, 1);
  }

  const bool reverseBytes = (fileLittleEndian != viskores::io::internal::IsLittleEndian());
  for (const T& value : values)
  {
    const char* bytes = reinterpret_cast<const char*>(&value);
    if (reverseBytes && sizeof(T) > 1)
    {
      for (std::size_t i = 0; i < sizeof(T); ++i)
        out.write(bytes + (sizeof(T) - 1 - i), 1);
    }
    else
      out.write(bytes, static_cast<std::streamsize>(sizeof(T)));
  }
  VISKORES_TEST_ASSERT(out.good(), "Unable to write test binary file");
}

template <typename T>
void CheckScalarField(const viskores::cont::DataSet& ds,
                      const std::string& fieldName,
                      viskores::cont::Field::Association association,
                      const std::vector<T>& expected)
{
  const auto& field = ds.GetField(fieldName, association);
  VISKORES_TEST_ASSERT(field.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()),
                       "Incorrect field value count");

  auto array = field.GetData().template AsArrayHandle<viskores::cont::ArrayHandle<T>>();
  auto portal = array.ReadPortal();
  for (viskores::Id i = 0; i < static_cast<viskores::Id>(expected.size()); ++i)
    VISKORES_TEST_ASSERT(test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]),
                         "Incorrect scalar field value");
}

template <typename T>
void CheckVectorField(const viskores::cont::DataSet& ds,
                      const std::string& fieldName,
                      const std::vector<viskores::Vec<T, 3>>& expected)
{
  const auto& field = ds.GetPointField(fieldName);
  VISKORES_TEST_ASSERT(field.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()),
                       "Incorrect vector field value count");

  auto array =
    field.GetData().template AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec<T, 3>>>();
  auto portal = array.ReadPortal();
  for (viskores::Id i = 0; i < static_cast<viskores::Id>(expected.size()); ++i)
    VISKORES_TEST_ASSERT(test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]),
                         "Incorrect vector field value");
}

template <typename T>
void TestScalarFormat(const std::string& testName,
                      const std::string& bovFormat,
                      const std::vector<T>& values)
{
  const std::string dataFileName = TestFileName(testName + ".bof");
  const std::string bovFile = TestPath(testName + ".bov");
  const std::string dataFile = viskores::cont::testing::Testing::WriteDirPath(dataFileName);

  WriteBinaryFile(dataFile, values);
  WriteTextFile(bovFile,
                "DATA_FILE: \"" + dataFileName +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: " +
                  bovFormat +
                  "\n"
                  "CENTERING: NODAL\n"
                  "VARIABLE: \"values\"\n");

  auto ds = ReadBOVDataSet(bovFile.c_str());
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 4, "Incorrect scalar-format point count");
  CheckScalarField(ds, "values", viskores::cont::Field::Association::Points, values);
}

void ExpectReadFails(const std::string& bovFile)
{
  try
  {
    viskores::io::BOVDataSetReader reader(bovFile);
    reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO&)
  {
    return;
  }

  VISKORES_TEST_FAIL("Expected BOV read to fail");
}

void TestReadingBundledBOVDataSet()
{
  std::string bovFile =
    viskores::cont::testing::Testing::DataPath("third_party/visit/example_temp.bov");

  auto const& ds = ReadBOVDataSet(bovFile.data());

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 50 * 50 * 50, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 50 * 50 * 50,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 49 * 49 * 49, "Incorrect number of cells");
  VISKORES_TEST_ASSERT(ds.HasField("var"), "Should have field 'var', but does not.");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "There is only one coordinate system in example_temp.bov");

  auto const& field = ds.GetField("var");
  VISKORES_TEST_ASSERT(field.GetAssociation() == viskores::cont::Field::Association::Points,
                       "The field should be associated with points.");
}

void TestScalarFormats()
{
  TestScalarFormat("scalar_byte", "BYTE", std::vector<viskores::UInt8>{ 1, 2, 3, 4 });
  TestScalarFormat("scalar_short", "SHORT", std::vector<viskores::Int16>{ -1, 2, -3, 4 });
  TestScalarFormat(
    "scalar_integer", "INTEGER", std::vector<viskores::Int32>{ -100000, 2, 3000, 400000 });
  TestScalarFormat(
    "scalar_float", "FLOAT", std::vector<viskores::Float32>{ 1.5f, 2.5f, 3.5f, 4.5f });
  TestScalarFormat(
    "scalar_double", "DOUBLE", std::vector<viskores::Float64>{ 1.25, 2.25, 3.25, 4.25 });
}

void TestVectorComponents()
{
  const std::string dataFileName = TestFileName("vector_float.bof");
  const std::string bovFile = TestPath("vector_float.bov");
  const std::string dataFile = viskores::cont::testing::Testing::WriteDirPath(dataFileName);

  WriteBinaryFile(dataFile,
                  std::vector<viskores::Float32>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 });
  WriteTextFile(bovFile,
                "DATA_FILE: \"" + dataFileName +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: FLOAT\n"
                  "DATA_COMPONENTS: 3\n"
                  "VARIABLE: \"vectors\"\n");

  auto ds = ReadBOVDataSet(bovFile.c_str());
  CheckVectorField(ds,
                   "vectors",
                   std::vector<viskores::Vec3f_32>{ viskores::Vec3f_32(1, 2, 3),
                                                    viskores::Vec3f_32(4, 5, 6),
                                                    viskores::Vec3f_32(7, 8, 9),
                                                    viskores::Vec3f_32(10, 11, 12) });
}

void TestEndianAndByteOffset()
{
  const bool fileLittleEndian = !viskores::io::internal::IsLittleEndian();
  const std::string endianName = fileLittleEndian ? "LITTLE" : "BIG";
  const std::string dataFileName = TestFileName("offset_endian.bof");
  const std::string bovFile = TestPath("offset_endian.bov");
  const std::string dataFile = viskores::cont::testing::Testing::WriteDirPath(dataFileName);
  const std::vector<viskores::Int32> values{ 0x01020304, -17, 1000000, 42 };

  WriteBinaryFile(dataFile, values, fileLittleEndian, std::vector<viskores::UInt8>{ 0, 1, 2, 3 });
  WriteTextFile(bovFile,
                "DATA_FILE: \"" + dataFileName +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: INTEGER\n"
                  "DATA_ENDIAN: " +
                  endianName +
                  "\n"
                  "BYTE_OFFSET: 4\n"
                  "VARIABLE: \"values\"\n");

  auto ds = ReadBOVDataSet(bovFile.c_str());
  CheckScalarField(ds, "values", viskores::cont::Field::Association::Points, values);
}

void TestZonalCentering()
{
  const std::string dataFileName = TestFileName("zonal_float.bof");
  const std::string bovFile = TestPath("zonal_float.bov");
  const std::string dataFile = viskores::cont::testing::Testing::WriteDirPath(dataFileName);
  const std::vector<viskores::Float32> values{ 1, 2, 3, 4, 5, 6 };

  WriteBinaryFile(dataFile, values);
  WriteTextFile(bovFile,
                "DATA_FILE: \"" + dataFileName +
                  "\"\n"
                  "DATA_SIZE: 2 3 1\n"
                  "DATA_FORMAT: FLOAT\n"
                  "CENTERING: ZONAL\n"
                  "BRICK_ORIGIN: -1 -2 -3\n"
                  "BRICK_SIZE: 2 3 1\n"
                  "VARIABLE: \"zonevar\"\n");

  auto ds = ReadBOVDataSet(bovFile.c_str());
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 6, "Incorrect zonal cell count");
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 3 * 4 * 2, "Incorrect zonal point count");
  CheckScalarField(ds, "zonevar", viskores::cont::Field::Association::Cells, values);

  const viskores::Bounds expectedBounds(-1, 1, -2, 1, -3, -2);
  VISKORES_TEST_ASSERT(test_equal(ds.GetCoordinateSystem().GetBounds(), expectedBounds),
                       "Incorrect zonal bounds");
}

void TestAbsoluteDataFile()
{
  const std::string bovFile = TestPath("absolute_path.bov");
  const std::string dataFile = viskores::io::MakeAbsolutePath(TestPath("absolute_path.bof"));
  const std::vector<viskores::Float64> values{ 9.0, 8.0, 7.0, 6.0 };

  WriteBinaryFile(dataFile, values);
  WriteTextFile(bovFile,
                "DATA_FILE: \"" + dataFile +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: DOUBLE\n"
                  "VARIABLE: \"absolute\"\n");

  auto ds = ReadBOVDataSet(bovFile.c_str());
  CheckScalarField(ds, "absolute", viskores::cont::Field::Association::Points, values);
}

void TestBadHeaders()
{
  const std::string dataFile = TestPath("bad_headers.bof");
  WriteBinaryFile(dataFile, std::vector<viskores::Float32>{ 1, 2, 3, 4 });

  const std::string missingDataFile = TestPath("missing_data_file.bov");
  WriteTextFile(missingDataFile,
                "DATA_SIZE: 2 2 1\n"
                "DATA_FORMAT: FLOAT\n"
                "VARIABLE: \"bad\"\n");
  ExpectReadFails(missingDataFile);

  const std::string badComponents = TestPath("bad_components.bov");
  WriteTextFile(badComponents,
                "DATA_FILE: \"" + TestFileName("bad_headers.bof") +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: FLOAT\n"
                  "DATA_COMPONENTS: abc\n"
                  "VARIABLE: \"bad\"\n");
  ExpectReadFails(badComponents);

  const std::string unsupportedComponents = TestPath("unsupported_components.bov");
  WriteTextFile(unsupportedComponents,
                "DATA_FILE: \"" + TestFileName("bad_headers.bof") +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: FLOAT\n"
                  "DATA_COMPONENTS: 2\n"
                  "VARIABLE: \"bad\"\n");
  ExpectReadFails(unsupportedComponents);

  const std::string dividedBrick = TestPath("divided_brick.bov");
  WriteTextFile(dividedBrick,
                "DATA_FILE: \"" + TestFileName("bad_headers.bof") +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: FLOAT\n"
                  "DIVIDE_BRICK: TRUE\n"
                  "VARIABLE: \"bad\"\n");
  ExpectReadFails(dividedBrick);

  const std::string truncatedData = TestPath("truncated_data.bov");
  const std::string truncatedBof = TestPath("truncated_data.bof");
  WriteBinaryFile(truncatedBof, std::vector<viskores::Float32>{ 1 });
  WriteTextFile(truncatedData,
                "DATA_FILE: \"" + TestFileName("truncated_data.bof") +
                  "\"\n"
                  "DATA_SIZE: 2 2 1\n"
                  "DATA_FORMAT: FLOAT\n"
                  "VARIABLE: \"bad\"\n");
  ExpectReadFails(truncatedData);
}

void TestReadingBOVDataSet()
{
  TestReadingBundledBOVDataSet();
  TestScalarFormats();
  TestVectorComponents();
  TestEndianAndByteOffset();
  TestZonalCentering();
  TestAbsoluteDataFile();
  TestBadHeaders();
}

} // anonymous namespace

int UnitTestBOVDataSetReader(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestReadingBOVDataSet, argc, argv);
}
