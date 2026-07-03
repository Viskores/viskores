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

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>
#include <viskores/rendering/anari-device/ViskoresDevice.h>
#include <viskores/rendering/anari-device/ViskoresDeviceGlobalState.h>

#include <anari/frontend/type_utility.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace
{

struct ArrayRecord
{
  ANARIDataType ElementType{ ANARI_UNKNOWN };
  viskores::UInt64 NumberOfItems{ 0 };
  std::vector<viskores::UInt8> Values;
};

struct ValueRecord
{
  ANARIDataType Type{ ANARI_UNKNOWN };
  std::vector<viskores::UInt8> Values;
};

class InspectionDevice : public viskores_device::ViskoresDevice
{
public:
  InspectionDevice()
    : viskores_device::ViskoresDevice(nullptr, nullptr)
  {
  }

  ANARIArray1D newArray1D(const void* appMemory,
                          ANARIMemoryDeleter deleter,
                          const void* userData,
                          ANARIDataType elementType,
                          uint64_t numberOfItems) override
  {
    auto array =
      this->ViskoresDevice::newArray1D(appMemory, deleter, userData, elementType, numberOfItems);
    ArrayRecord record;
    record.ElementType = elementType;
    record.NumberOfItems = numberOfItems;
    record.Values.resize(anari::sizeOf(elementType) * numberOfItems);
    if (appMemory)
    {
      std::memcpy(record.Values.data(), appMemory, record.Values.size());
    }
    this->Arrays[array] = std::move(record);
    return array;
  }

  void* mapArray(ANARIArray array) override
  {
    auto* mapped = this->ViskoresDevice::mapArray(array);
    this->MappedArrays[array] = mapped;
    return mapped;
  }

  void unmapArray(ANARIArray array) override
  {
    auto record = this->Arrays.find(reinterpret_cast<ANARIArray1D>(array));
    auto mapped = this->MappedArrays.find(array);
    if (record != this->Arrays.end() && mapped != this->MappedArrays.end())
    {
      std::memcpy(record->second.Values.data(), mapped->second, record->second.Values.size());
    }
    this->MappedArrays.erase(array);
    this->ViskoresDevice::unmapArray(array);
  }

  void setParameter(ANARIObject object,
                    const char* name,
                    ANARIDataType type,
                    const void* value) override
  {
    if (anari::isObject(type) && value)
    {
      auto parameter = *static_cast<ANARIObject const*>(value);
      this->ObjectParameters[object][name] = parameter;
      if (type == ANARI_ARRAY1D)
      {
        auto array = this->Arrays.find(reinterpret_cast<ANARIArray1D>(parameter));
        if (array != this->Arrays.end())
        {
          this->ArrayParameters[object][name] = array->second;
        }
      }
    }
    else
    {
      this->ObjectParameters[object].erase(name);
    }
    if (type == ANARI_STRING && value)
    {
      this->StringParameters[object][name] = static_cast<const char*>(value);
    }
    if (!anari::isObject(type) && type != ANARI_STRING && value)
    {
      ValueRecord record;
      record.Type = type;
      record.Values.resize(anari::sizeOf(type));
      std::memcpy(record.Values.data(), value, record.Values.size());
      this->ValueParameters[object][name] = std::move(record);
    }
    this->helium::BaseDevice::setParameter(object, name, type, value);
  }

  void unsetParameter(ANARIObject object, const char* name) override
  {
    this->ObjectParameters[object].erase(name);
    this->ArrayParameters[object].erase(name);
    this->StringParameters[object].erase(name);
    this->ValueParameters[object].erase(name);
    this->helium::BaseDevice::unsetParameter(object, name);
  }

  ANARIObject GetObjectParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->ObjectParameters.find(object);
    VISKORES_TEST_ASSERT(objectParameters != this->ObjectParameters.end(),
                         "No object parameters were recorded for the ANARI object.");
    auto parameter = objectParameters->second.find(name);
    VISKORES_TEST_ASSERT(parameter != objectParameters->second.end(),
                         "The expected ANARI object parameter was not set: " + name);
    return parameter->second;
  }

  const ArrayRecord& GetArrayParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->ArrayParameters.find(object);
    VISKORES_TEST_ASSERT(objectParameters != this->ArrayParameters.end(),
                         "No array parameters were recorded for the ANARI object.");
    auto parameter = objectParameters->second.find(name);
    VISKORES_TEST_ASSERT(parameter != objectParameters->second.end(),
                         "The expected ANARI array parameter was not set: " + name);
    return parameter->second;
  }

  bool HasObjectParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->ObjectParameters.find(object);
    return objectParameters != this->ObjectParameters.end() &&
      objectParameters->second.find(name) != objectParameters->second.end();
  }

  const std::string& GetStringParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->StringParameters.find(object);
    VISKORES_TEST_ASSERT(objectParameters != this->StringParameters.end(),
                         "No string parameters were recorded for the ANARI object.");
    auto parameter = objectParameters->second.find(name);
    VISKORES_TEST_ASSERT(parameter != objectParameters->second.end(),
                         "The expected ANARI string parameter was not set: " + name);
    return parameter->second;
  }

  const ValueRecord& GetValueParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->ValueParameters.find(object);
    VISKORES_TEST_ASSERT(objectParameters != this->ValueParameters.end(),
                         "No value parameters were recorded for the ANARI object.");
    auto parameter = objectParameters->second.find(name);
    VISKORES_TEST_ASSERT(parameter != objectParameters->second.end(),
                         "The expected ANARI value parameter was not set: " + name);
    return parameter->second;
  }

  std::size_t GetNumberOfLiveArrays() const
  {
    auto* state = static_cast<viskores_device::ViskoresDeviceGlobalState*>(this->m_state.get());
    return state->objectCounts.arrays.load();
  }

  void Flush()
  {
    auto* state = static_cast<viskores_device::ViskoresDeviceGlobalState*>(this->m_state.get());
    state->commitBuffer.flush();
  }

  void ClearPendingCommits()
  {
    auto* state = static_cast<viskores_device::ViskoresDeviceGlobalState*>(this->m_state.get());
    state->commitBuffer.clear();
  }

private:
  std::map<ANARIArray1D, ArrayRecord> Arrays;
  std::map<ANARIArray, void*> MappedArrays;
  std::map<ANARIObject, std::map<std::string, ANARIObject>> ObjectParameters;
  std::map<ANARIObject, std::map<std::string, ArrayRecord>> ArrayParameters;
  std::map<ANARIObject, std::map<std::string, std::string>> StringParameters;
  std::map<ANARIObject, std::map<std::string, ValueRecord>> ValueParameters;
};

template <typename ValueType, typename RecordType>
std::vector<ValueType> ReadValues(const RecordType& record)
{
  VISKORES_TEST_ASSERT(record.Values.size() % sizeof(ValueType) == 0,
                       "Recorded ANARI parameter has an unexpected size.");
  std::vector<ValueType> values(record.Values.size() / sizeof(ValueType));
  std::memcpy(values.data(), record.Values.data(), record.Values.size());
  return values;
}

viskores::Vec4f_32 FirstSample(const viskores::cont::ColorTable& colorTable)
{
  viskores::cont::ArrayHandle<viskores::Vec4ui_8> samples;
  VISKORES_TEST_ASSERT(colorTable.Sample(256, samples),
                       "The test ColorTable could not be sampled.");
  const auto sample = samples.ReadPortal().Get(0);
  constexpr viskores::Float32 normalize = 1.f / 255.f;
  return {
    sample[0] * normalize, sample[1] * normalize, sample[2] * normalize, sample[3] * normalize
  };
}

viskores::interop::anari::ANARIActor MakePointActor()
{
  std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f },
                                             { 1.f, 0.f, 0.f },
                                             { 0.f, 1.f, 0.f } };
  std::vector<viskores::Float32> values{ -2.f, 1.f, 4.f };
  viskores::cont::CoordinateSystem coordinates(
    "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  return { {}, coordinates, field };
}

viskores::interop::anari::ANARIActor MakeTwoFieldPointActor(viskores::IdComponent primaryField)
{
  std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f },
                                             { 1.f, 0.f, 0.f },
                                             { 0.f, 1.f, 0.f } };
  std::vector<viskores::Float32> firstValues{ -2.f, 1.f, 4.f };
  std::vector<viskores::Float32> secondValues{ 10.f, 20.f, 30.f };
  viskores::cont::CoordinateSystem coordinates(
    "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
  viskores::cont::Field firstField(
    "first",
    viskores::cont::Field::Association::Points,
    viskores::cont::make_ArrayHandle(firstValues, viskores::CopyFlag::On));
  viskores::cont::Field secondField(
    "second",
    viskores::cont::Field::Association::Points,
    viskores::cont::make_ArrayHandle(secondValues, viskores::CopyFlag::On));
  viskores::interop::anari::ANARIActor actor({}, coordinates, firstField, secondField);
  actor.SetPrimaryFieldIndex(primaryField);
  return actor;
}

viskores::interop::anari::ANARIActor MakeTriangleActor()
{
  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id2{ 2, 2 });
  std::vector<viskores::Float32> values{ -2.f, 1.f, 3.f, 4.f };
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  return { dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field };
}

viskores::interop::anari::ANARIActor MakeVolumeActor()
{
  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3{ 2, 2, 2 });
  std::vector<viskores::Float32> values{ -2.f, -1.f, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f };
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  return { dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field };
}

void TestConstructorColorTableIsSampled()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::cont::ColorTable colorTable(viskores::cont::ColorTable::Preset::CoolToWarm);
    viskores::interop::anari::ANARIMapperPoints mapper(
      device, MakePointActor(), "sampled", colorTable);
    auto surface = mapper.GetANARISurface();
    auto material = inspection->GetObjectParameter(surface, "material");
    auto sampler = inspection->GetObjectParameter(material, "color");
    const auto& image = inspection->GetArrayParameter(sampler, "image");

    VISKORES_TEST_ASSERT(image.ElementType == ANARI_FLOAT32_VEC4,
                         "The surface color table was not an RGBA float array.");
    VISKORES_TEST_ASSERT(image.NumberOfItems == 256,
                         "The supplied ColorTable was not sampled into the ANARI image.");
    const auto colors = ReadValues<viskores::Vec4f_32>(image);
    VISKORES_TEST_ASSERT(test_equal(colors.front(), FirstSample(colorTable)),
                         "The sampled ANARI image did not contain the supplied ColorTable.");
  }
  anari_cpp::release(device, device);
}

void TestColorTableUpdatesMaterializedSampler()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::interop::anari::ANARIMapperPoints mapper(device, MakePointActor());
    viskores::cont::ColorTable initial(viskores::cont::ColorTable::Preset::CoolToWarm);
    mapper.SetColorTable(initial);
    auto surface = mapper.GetANARISurface();
    auto material = inspection->GetObjectParameter(surface, "material");
    auto sampler = inspection->GetObjectParameter(material, "color");
    auto colors = ReadValues<viskores::Vec4f_32>(inspection->GetArrayParameter(sampler, "image"));
    VISKORES_TEST_ASSERT(test_equal(colors.front(), FirstSample(initial)),
                         "A pre-materialization SetColorTable call was ignored.");

    viskores::cont::ColorTable replacement(viskores::cont::ColorTable::Preset::BlackBodyRadiation);
    mapper.SetColorTable(replacement);

    colors = ReadValues<viskores::Vec4f_32>(inspection->GetArrayParameter(sampler, "image"));
    VISKORES_TEST_ASSERT(test_equal(colors.front(), FirstSample(replacement)),
                         "SetColorTable did not update an existing ANARI sampler.");
  }
  anari_cpp::release(device, device);
}

void TestNameUpdatesBeforeAndAfterMaterialization()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::interop::anari::ANARIMapperPoints mapper(device, MakePointActor());
    mapper.SetName("before");
    auto surface = mapper.GetANARISurface();
    auto geometry = mapper.GetANARIGeometry();
    auto material = inspection->GetObjectParameter(surface, "material");
    auto sampler = inspection->GetObjectParameter(material, "color");
    auto group = mapper.GetANARIGroup();
    auto instance = mapper.GetANARIInstance();

    VISKORES_TEST_ASSERT(inspection->GetStringParameter(surface, "name") == "before.surface" &&
                           inspection->GetStringParameter(geometry, "name") == "before.geometry" &&
                           inspection->GetStringParameter(material, "name") == "before.material" &&
                           inspection->GetStringParameter(sampler, "name") == "before.colormap" &&
                           inspection->GetStringParameter(group, "name") == "before.group" &&
                           inspection->GetStringParameter(instance, "name") == "before.instance",
                         "A pre-materialization name was not applied to every ANARI object.");

    mapper.SetName("after");
    VISKORES_TEST_ASSERT(inspection->GetStringParameter(surface, "name") == "after.surface" &&
                           inspection->GetStringParameter(geometry, "name") == "after.geometry" &&
                           inspection->GetStringParameter(material, "name") == "after.material" &&
                           inspection->GetStringParameter(sampler, "name") == "after.colormap" &&
                           inspection->GetStringParameter(group, "name") == "after.group" &&
                           inspection->GetStringParameter(instance, "name") == "after.instance",
                         "SetName did not update every materialized ANARI object.");
  }
  anari_cpp::release(device, device);
}

template <typename MapperType>
void CheckDegenerateRangeTransforms(InspectionDevice* inspection,
                                    anari_cpp::Device device,
                                    const viskores::interop::anari::ANARIActor& actor)
{
  MapperType mapper(device, actor);
  mapper.SetANARIColorMapValueRange({ 4.f, 4.f });
  auto surface = mapper.GetANARISurface();
  auto material = inspection->GetObjectParameter(surface, "material");
  auto sampler = inspection->GetObjectParameter(material, "color");

  auto transform =
    ReadValues<viskores::Float32>(inspection->GetValueParameter(sampler, "inTransform"));
  VISKORES_TEST_ASSERT(
    std::all_of(
      transform.begin(), transform.end(), [](auto value) { return std::isfinite(value); }),
    "A pre-materialization degenerate range produced a non-finite sampler transform.");

  mapper.SetANARIColorMapValueRange({ -3.f, -3.f });
  transform = ReadValues<viskores::Float32>(inspection->GetValueParameter(sampler, "inTransform"));
  VISKORES_TEST_ASSERT(
    std::all_of(
      transform.begin(), transform.end(), [](auto value) { return std::isfinite(value); }),
    "A post-materialization degenerate range produced a non-finite sampler transform.");
}

void TestDegenerateSurfaceRangesStayFinite()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    CheckDegenerateRangeTransforms<viskores::interop::anari::ANARIMapperPoints>(
      inspection, device, MakePointActor());
    CheckDegenerateRangeTransforms<viskores::interop::anari::ANARIMapperTriangles>(
      inspection, device, MakeTriangleActor());
  }
  anari_cpp::release(device, device);
}

void TestVolumeRangeAndOpacityUpdates()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::interop::anari::ANARIMapperVolume mapper(device, MakeVolumeActor());
    mapper.SetANARIColorMapOpacityScale(2.5f);
    auto volume = mapper.GetANARIVolume();

    auto range = ReadValues<viskores::Float32>(inspection->GetValueParameter(volume, "valueRange"));
    auto opacityScale =
      ReadValues<viskores::Float32>(inspection->GetValueParameter(volume, "densityScale"));
    const auto& colors = inspection->GetArrayParameter(volume, "color");
    const auto& opacities = inspection->GetArrayParameter(volume, "opacity");
    VISKORES_TEST_ASSERT(range == std::vector<viskores::Float32>{ -2.f, 5.f },
                         "The default volume range was not derived from the primary field.");
    VISKORES_TEST_ASSERT(colors.ElementType == ANARI_FLOAT32_VEC3 && colors.NumberOfItems == 256 &&
                           opacities.ElementType == ANARI_FLOAT32 && opacities.NumberOfItems == 256,
                         "The volume did not use sampled ColorTable arrays.");
    VISKORES_TEST_ASSERT(opacityScale == std::vector<viskores::Float32>{ 2.5f },
                         "A pre-materialization opacity scale was not applied.");

    mapper.SetANARIColorMapValueRange({ 10.f, 20.f });
    mapper.SetANARIColorMapOpacityScale(0.25f);
    range = ReadValues<viskores::Float32>(inspection->GetValueParameter(volume, "valueRange"));
    opacityScale =
      ReadValues<viskores::Float32>(inspection->GetValueParameter(volume, "densityScale"));
    VISKORES_TEST_ASSERT(range == std::vector<viskores::Float32>{ 10.f, 20.f },
                         "SetANARIColorMapValueRange did not update a materialized volume.");
    VISKORES_TEST_ASSERT(opacityScale == std::vector<viskores::Float32>{ 0.25f },
                         "SetANARIColorMapOpacityScale did not update a materialized volume.");
  }
  anari_cpp::release(device, device);
}

void TestMapFieldAndPrimaryFieldUpdates()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeTwoFieldPointActor(1);
    viskores::interop::anari::ANARIMapperPoints mapper(device, actor);
    mapper.SetMapFieldAsAttribute(false);
    auto surface = mapper.GetANARISurface();
    auto geometry = mapper.GetANARIGeometry();
    auto material = inspection->GetObjectParameter(surface, "material");
    VISKORES_TEST_ASSERT(!inspection->HasObjectParameter(material, "color"),
                         "A pre-materialization map-field disable was ignored.");
    VISKORES_TEST_ASSERT(!inspection->HasObjectParameter(geometry, "vertex.attribute1"),
                         "Disabled field mapping still exposed geometry attributes.");

    mapper.SetMapFieldAsAttribute(true);
    auto sampler = inspection->GetObjectParameter(material, "color");
    VISKORES_TEST_ASSERT(
      inspection->HasObjectParameter(geometry, "vertex.attribute1") &&
        inspection->GetStringParameter(sampler, "inAttribute") == "attribute1",
      "Enabling field mapping did not update the existing geometry and material.");

    actor.SetPrimaryFieldIndex(0);
    mapper.SetActor(actor);
    VISKORES_TEST_ASSERT(inspection->GetStringParameter(sampler, "inAttribute") == "attribute0",
                         "Changing the primary field did not update the material input.");

    mapper.SetMapFieldAsAttribute(false);
    VISKORES_TEST_ASSERT(!inspection->HasObjectParameter(material, "color") &&
                           !inspection->HasObjectParameter(geometry, "vertex.attribute0"),
                         "Disabling field mapping did not update materialized ANARI objects.");
  }
  anari_cpp::release(device, device);
}

void TestRawArrayOwnershipTransfer()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  auto color = anari_cpp::newArray1D(device, ANARI_FLOAT32_VEC3, 2);
  auto opacity = anari_cpp::newArray1D(device, ANARI_FLOAT32, 2);
  {
    viskores::interop::anari::ANARIMapperVolume mapper(device, MakeVolumeActor());
    mapper.SetANARIColorMap(color, opacity, true);
    auto volume = mapper.GetANARIVolume();
    VISKORES_TEST_ASSERT(inspection->GetObjectParameter(volume, "color") == color &&
                           inspection->GetObjectParameter(volume, "opacity") == opacity,
                         "Transferred raw arrays were not applied after materialization.");
  }
  inspection->ClearPendingCommits();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                       "Transferred raw arrays outlived their mapper: " +
                         std::to_string(inspection->GetNumberOfLiveArrays()));
  anari_cpp::release(device, device);
}

void TestRawArrayCallerOwnership()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  auto color = anari_cpp::newArray1D(device, ANARI_FLOAT32_VEC3, 2);
  auto opacity = anari_cpp::newArray1D(device, ANARI_FLOAT32, 2);
  {
    viskores::interop::anari::ANARIMapperVolume mapper(device, MakeVolumeActor());
    mapper.SetANARIColorMap(color, opacity, false);
    auto volume = mapper.GetANARIVolume();
    VISKORES_TEST_ASSERT(inspection->GetObjectParameter(volume, "color") == color &&
                           inspection->GetObjectParameter(volume, "opacity") == opacity,
                         "Retained raw arrays were not applied after materialization.");
  }
  inspection->ClearPendingCommits();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 2,
                       "The mapper released caller-owned raw array references.");
  anari_cpp::release(device, color);
  anari_cpp::release(device, opacity);
  inspection->ClearPendingCommits();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                       "Caller-owned raw arrays were not released by their owner.");
  anari_cpp::release(device, device);
}

void RunTests()
{
  TestConstructorColorTableIsSampled();
  TestColorTableUpdatesMaterializedSampler();
  TestNameUpdatesBeforeAndAfterMaterialization();
  TestDegenerateSurfaceRangesStayFinite();
  TestVolumeRangeAndOpacityUpdates();
  TestMapFieldAndPrimaryFieldUpdates();
  TestRawArrayOwnershipTransfer();
  TestRawArrayCallerOwnership();
}

} // namespace

int UnitTestANARIMapperAppearance(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
