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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>

#include <string>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

template <typename Callable>
void ExpectBadValue(Callable&& callable, const std::string& expectedMessage)
{
  bool rejected = false;
  try
  {
    callable();
  }
  catch (const viskores::cont::ErrorBadValue& error)
  {
    rejected = true;
    VISKORES_TEST_ASSERT(error.GetMessage().find(expectedMessage) != std::string::npos,
                         "Unexpected error message: " + error.GetMessage());
  }

  VISKORES_TEST_ASSERT(rejected, "Expected ANARI volume input to be rejected.");
}

bool StringListContains(const char* const* values, const std::string& expected)
{
  if (!values)
  {
    return false;
  }
  for (const char* const* value = values; *value != nullptr; ++value)
  {
    if (*value == expected)
    {
      return true;
    }
  }
  return false;
}

void TestStructuredRegularSupportAdvertised()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  const char** extensions = nullptr;
  anariGetProperty(
    device, device, "extension", ANARI_STRING_LIST, &extensions, sizeof(extensions), ANARI_WAIT);
  VISKORES_TEST_ASSERT(StringListContains(extensions, "ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR"),
                       "The device did not advertise structuredRegular extension support.");

  const char** spatialFieldSubtypes = anariGetObjectSubtypes(device, ANARI_SPATIAL_FIELD);
  VISKORES_TEST_ASSERT(StringListContains(spatialFieldSubtypes, "structuredRegular"),
                       "The device did not advertise the structuredRegular subtype.");
}

void TestCompletelyEmptyActorProducesEmptyMapper()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();
  viskores::interop::anari::ANARIMapperVolume mapper(device);

  VISKORES_TEST_ASSERT(mapper.GetANARISpatialField() == nullptr,
                       "An empty mapper created a spatial field.");
  VISKORES_TEST_ASSERT(mapper.GetANARIVolume() == nullptr, "An empty mapper created a volume.");
  VISKORES_TEST_ASSERT(mapper.GroupIsEmpty(), "An empty mapper reported renderable content.");
}

void TestExplicitCoordinatesRejected()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  const viskores::Id3 dimensions{ 2, 2, 2 };
  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dimensions);

  std::vector<viskores::Vec3f> coordinates;
  coordinates.reserve(8);
  for (viskores::Id zIndex = 0; zIndex < dimensions[2]; ++zIndex)
  {
    for (viskores::Id yIndex = 0; yIndex < dimensions[1]; ++yIndex)
    {
      for (viskores::Id xIndex = 0; xIndex < dimensions[0]; ++xIndex)
      {
        coordinates.emplace_back(static_cast<viskores::FloatDefault>(xIndex),
                                 static_cast<viskores::FloatDefault>(yIndex),
                                 static_cast<viskores::FloatDefault>(zIndex));
      }
    }
  }

  std::vector<viskores::Float32> values(8, 1.f);
  viskores::cont::CoordinateSystem explicitCoordinates(
    "coordinates", viskores::cont::make_ArrayHandle(coordinates, viskores::CopyFlag::On));
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor actor(dataSet.GetCellSet(), explicitCoordinates, field);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

  ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "uniform point coordinates");
}

void TestCellFieldRejected()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3{ 3, 3, 3 });
  std::vector<viskores::Float32> values(8, 1.f);
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Cells,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

  ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "point-associated scalar field");
}

void TestVectorFieldRejected()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3{ 2, 2, 2 });
  std::vector<viskores::Vec3f_32> values(8, viskores::Vec3f_32{ 1.f, 2.f, 3.f });
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

  ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "point-associated scalar field");
}

void TestEmptyFieldRejected()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3{ 2, 2, 2 });
  viskores::cont::ArrayHandle<viskores::Float32> values;
  viskores::cont::Field field("values", viskores::cont::Field::Association::Points, values);
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

  ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "non-empty field");
}

void TestDegenerateDimensionsRejected()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  const viskores::Id3 dimensions{ 1, 2, 2 };
  viskores::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(dimensions);
  viskores::cont::CoordinateSystem coordinates("coordinates", dimensions);
  std::vector<viskores::Float32> values(4, 1.f);
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor actor(cellSet, coordinates, field);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

  ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "at least two samples per dimension");
}

void TestUniformOriginAndSpacing()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  const viskores::Id3 dimensions{ 3, 4, 2 };
  const viskores::Vec3f origin{ 5.f, -2.f, 10.f };
  const viskores::Vec3f spacing{ 0.5f, 2.f, 3.f };
  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dimensions, origin, spacing);
  std::vector<viskores::Float32> values(24, 1.f);
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

  auto world = anari_cpp::newObject<anari_cpp::World>(device);
  auto volume = mapper.GetANARIVolume();
  anari_cpp::setParameterArray1D(device, world, "volume", &volume, 1);
  anari_cpp::commitParameters(device, world);

  viskores::Vec3f_32 bounds[2];
  const auto boundsAvailable = anariGetProperty(
    device, world, "bounds", ANARI_FLOAT32_BOX3, bounds, sizeof(bounds), ANARI_WAIT);
  VISKORES_TEST_ASSERT(boundsAvailable != 0, "The ANARI device did not provide world bounds.");
  VISKORES_TEST_ASSERT(test_equal(bounds[0], viskores::Vec3f_32{ 5.f, -2.f, 10.f }),
                       "The structured volume origin was not preserved.");
  VISKORES_TEST_ASSERT(test_equal(bounds[1], viskores::Vec3f_32{ 6.f, 4.f, 13.f }),
                       "The structured volume spacing was not preserved.");

  anari_cpp::release(device, world);
}

void TestReplacingActorUpdatesExistingVolume()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  auto makeActor = [](const viskores::Id3& dimensions,
                      const viskores::Vec3f& origin,
                      const viskores::Vec3f& spacing)
  {
    auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dimensions, origin, spacing);
    std::vector<viskores::Float32> values(
      static_cast<std::size_t>(dimensions[0] * dimensions[1] * dimensions[2]), 1.f);
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    return viskores::interop::anari::ANARIActor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field);
  };

  auto originalActor =
    makeActor(viskores::Id3{ 2, 2, 2 }, viskores::Vec3f{ 0.f, 0.f, 0.f }, viskores::Vec3f{ 1.f });
  viskores::interop::anari::ANARIMapperVolume mapper(device, originalActor);
  auto originalVolume = mapper.GetANARIVolume();

  auto world = anari_cpp::newObject<anari_cpp::World>(device);
  anari_cpp::setParameterArray1D(device, world, "volume", &originalVolume, 1);
  anari_cpp::commitParameters(device, world);

  auto replacementActor = makeActor(viskores::Id3{ 3, 2, 2 },
                                    viskores::Vec3f{ 10.f, 20.f, 30.f },
                                    viskores::Vec3f{ 2.f, 3.f, 4.f });
  mapper.SetActor(replacementActor);

  VISKORES_TEST_ASSERT(mapper.GetANARIVolume() == originalVolume,
                       "Replacing an actor replaced the ANARI volume handle.");
  viskores::Vec3f_32 bounds[2];
  const auto boundsAvailable = anariGetProperty(
    device, world, "bounds", ANARI_FLOAT32_BOX3, bounds, sizeof(bounds), ANARI_WAIT);
  VISKORES_TEST_ASSERT(boundsAvailable != 0, "The ANARI device did not provide world bounds.");
  VISKORES_TEST_ASSERT(test_equal(bounds[0], viskores::Vec3f_32{ 10.f, 20.f, 30.f }),
                       "Replacing the actor did not update the structured volume origin.");
  VISKORES_TEST_ASSERT(test_equal(bounds[1], viskores::Vec3f_32{ 14.f, 23.f, 34.f }),
                       "Replacing the actor did not update the structured volume spacing.");

  anari_cpp::release(device, world);
}

void TestRejectedReplacementPreservesExistingState()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3{ 2, 2, 2 });
  std::vector<viskores::Float32> scalarValues(8, 1.f);
  viskores::cont::Field scalarField(
    "scalars",
    viskores::cont::Field::Association::Points,
    viskores::cont::make_ArrayHandle(scalarValues, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor originalActor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), scalarField);
  viskores::interop::anari::ANARIMapperVolume mapper(device, originalActor);
  auto originalSpatialField = mapper.GetANARISpatialField();

  std::vector<viskores::Vec3f_32> vectorValues(8, viskores::Vec3f_32{ 1.f, 2.f, 3.f });
  viskores::cont::Field vectorField(
    "vectors",
    viskores::cont::Field::Association::Points,
    viskores::cont::make_ArrayHandle(vectorValues, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor rejectedActor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), vectorField);

  ExpectBadValue([&] { mapper.SetActor(rejectedActor); }, "point-associated scalar field");
  VISKORES_TEST_ASSERT(mapper.GetActor().GetField().GetData().GetNumberOfComponentsFlat() == 1,
                       "A rejected replacement changed the mapper actor.");
  VISKORES_TEST_ASSERT(mapper.GetANARISpatialField() == originalSpatialField,
                       "A rejected replacement changed the mapper spatial field.");
}

void RunTests()
{
  TestStructuredRegularSupportAdvertised();
  TestCompletelyEmptyActorProducesEmptyMapper();
  TestExplicitCoordinatesRejected();
  TestCellFieldRejected();
  TestVectorFieldRejected();
  TestEmptyFieldRejected();
  TestDegenerateDimensionsRejected();
  TestUniformOriginAndSpacing();
  TestReplacingActorUpdatesExistingVolume();
  TestRejectedReplacementPreservesExistingState();
}

} // namespace

int UnitTestANARIStructuredVolume(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
