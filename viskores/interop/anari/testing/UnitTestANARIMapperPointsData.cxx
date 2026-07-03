//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/Math.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>
#include <viskores/rendering/anari-device/ViskoresDevice.h>

#include <anari/frontend/type_utility.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{

struct ArrayRecord
{
  const void* SourceMemory{ nullptr };
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
    record.SourceMemory = appMemory;
    record.ElementType = elementType;
    record.NumberOfItems = numberOfItems;
    record.Values.resize(anari::sizeOf(elementType) * numberOfItems);
    if (appMemory)
    {
      std::memcpy(record.Values.data(), appMemory, record.Values.size());
    }
    ++this->NumberOfArraysCreated;
    this->NumberOfArrayBytesCreated += record.Values.size();
    this->Arrays[array] = std::move(record);
    return array;
  }

  ANARIGeometry newGeometry(const char* subtype) override
  {
    auto geometry = this->ViskoresDevice::newGeometry(subtype);
    this->GeometrySubtypes[geometry] = subtype ? subtype : "";
    return geometry;
  }

  void setParameter(ANARIObject object,
                    const char* name,
                    ANARIDataType type,
                    const void* value) override
  {
    this->ParameterNames[object].insert(name);
    if (anari::isObject(type) && value)
    {
      this->ObjectParameters[object][name] = *static_cast<ANARIObject const*>(value);
    }
    else
    {
      this->ObjectParameters[object].erase(name);
    }
    if (type == ANARI_ARRAY1D && value)
    {
      auto array = *static_cast<ANARIArray1D const*>(value);
      auto record = this->Arrays.find(array);
      if (record != this->Arrays.end())
      {
        this->ArrayParameters[object][name] = record->second;
      }
    }
    if (type == ANARI_FLOAT32 && value)
    {
      this->FloatParameters[object][name] = *static_cast<const viskores::Float32*>(value);
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
    this->ParameterNames[object].erase(name);
    this->ArrayParameters[object].erase(name);
    this->FloatParameters[object].erase(name);
    this->StringParameters[object].erase(name);
    this->ObjectParameters[object].erase(name);
    this->ValueParameters[object].erase(name);
    this->helium::BaseDevice::unsetParameter(object, name);
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

  viskores::Float32 GetFloatParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->FloatParameters.find(object);
    VISKORES_TEST_ASSERT(objectParameters != this->FloatParameters.end(),
                         "No float parameters were recorded for the ANARI object.");
    auto parameter = objectParameters->second.find(name);
    VISKORES_TEST_ASSERT(parameter != objectParameters->second.end(),
                         "The expected ANARI float parameter was not set: " + name);
    return parameter->second;
  }

  bool HasParameter(ANARIObject object, const std::string& name) const
  {
    auto parameterNames = this->ParameterNames.find(object);
    return parameterNames != this->ParameterNames.end() &&
      parameterNames->second.find(name) != parameterNames->second.end();
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

  bool HasObjectParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->ObjectParameters.find(object);
    return objectParameters != this->ObjectParameters.end() &&
      objectParameters->second.find(name) != objectParameters->second.end();
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

  std::size_t GetNumberOfArraysCreated() const { return this->NumberOfArraysCreated; }

  std::size_t GetNumberOfArrayBytesCreated() const { return this->NumberOfArrayBytesCreated; }

  const std::string& GetGeometrySubtype(ANARIGeometry geometry) const
  {
    auto subtype = this->GeometrySubtypes.find(geometry);
    VISKORES_TEST_ASSERT(subtype != this->GeometrySubtypes.end(),
                         "No subtype was recorded for the ANARI geometry.");
    return subtype->second;
  }

private:
  std::map<ANARIArray1D, ArrayRecord> Arrays;
  std::map<ANARIObject, std::map<std::string, ArrayRecord>> ArrayParameters;
  std::map<ANARIObject, std::map<std::string, viskores::Float32>> FloatParameters;
  std::map<ANARIObject, std::map<std::string, std::string>> StringParameters;
  std::map<ANARIObject, std::map<std::string, ANARIObject>> ObjectParameters;
  std::map<ANARIObject, std::map<std::string, ValueRecord>> ValueParameters;
  std::map<ANARIObject, std::set<std::string>> ParameterNames;
  std::map<ANARIGeometry, std::string> GeometrySubtypes;
  std::size_t NumberOfArraysCreated{ 0 };
  std::size_t NumberOfArrayBytesCreated{ 0 };
};

template <typename ValueType, typename RecordType>
std::vector<ValueType> ReadValues(const RecordType& record)
{
  VISKORES_TEST_ASSERT(record.Values.size() % sizeof(ValueType) == 0,
                       "Recorded ANARI array has an unexpected size.");
  std::vector<ValueType> values(record.Values.size() / sizeof(ValueType));
  std::memcpy(values.data(), record.Values.data(), record.Values.size());
  return values;
}

void TestContiguousPositionsAndGlobalRadius()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f },
                                               { 1.f, 0.f, 0.f },
                                               { 1.f, 2.f, 0.f } };
    auto positionArray = viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::Off);
    viskores::cont::CoordinateSystem coordinates("coordinates", positionArray);
    viskores::interop::anari::ANARIActor actor({}, coordinates);

    viskores::interop::anari::ANARIMapperPoints mapper(device, actor);
    auto geometry = mapper.GetANARIGeometry();

    VISKORES_TEST_ASSERT(inspection->GetGeometrySubtype(geometry) == "sphere",
                         "The point mapper did not create sphere geometry.");
    const auto& positionParameter = inspection->GetArrayParameter(geometry, "vertex.position");
    VISKORES_TEST_ASSERT(positionParameter.ElementType == ANARI_FLOAT32_VEC3,
                         "Point positions were not ANARI_FLOAT32_VEC3.");
    VISKORES_TEST_ASSERT(positionParameter.NumberOfItems == 3,
                         "Point positions did not preserve the coordinate count.");
    VISKORES_TEST_ASSERT(positionParameter.SourceMemory == positions.data(),
                         "Contiguous Float32 coordinates were copied before ANARI binding.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.radius"),
                         "A per-point radius array was allocated for a uniform radius.");
    VISKORES_TEST_ASSERT(inspection->GetFloatParameter(geometry, "radius") > 0.f,
                         "The point radius was not bound as a positive global scalar.");
  }
  anari_cpp::release(device, device);
}

void TestEmptyActorClearsGeometry()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f } };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperPoints mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates));
    auto geometry = mapper.GetANARIGeometry();
    VISKORES_TEST_ASSERT(inspection->HasParameter(geometry, "vertex.position"),
                         "A populated point actor did not bind positions.");

    mapper.SetActor({});
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.position"),
                         "Replacing a point actor with empty data left stale positions bound.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "radius"),
                         "Replacing a point actor with empty data left a stale radius bound.");
  }
  anari_cpp::release(device, device);
}

void TestPointAttributesPreserveTypesValuesAndNames()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f } };
    std::vector<viskores::Float64> scalarValues{ 1.5, 2.5 };
    std::vector<viskores::Vec<viskores::Int32, 2>> vector2Values{ { 1, 2 }, { 3, 4 } };
    std::vector<viskores::Vec<viskores::Float64, 3>> vector3Values{ { 5., 6., 7. },
                                                                    { 8., 9., 10. } };
    std::vector<viskores::Vec4f_32> vector4Values{ { 11.f, 12.f, 13.f, 14.f },
                                                   { 15.f, 16.f, 17.f, 18.f } };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field scalarField(
      "scalar",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(scalarValues, viskores::CopyFlag::On));
    viskores::cont::Field vector2Field(
      "vector2",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(vector2Values, viskores::CopyFlag::On));
    viskores::cont::Field vector3Field(
      "vector3",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(vector3Values, viskores::CopyFlag::On));
    viskores::cont::Field vector4Field(
      "vector4",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(vector4Values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(
      {}, coordinates, scalarField, vector2Field, vector3Field, vector4Field);

    viskores::interop::anari::ANARIMapperPoints mapper(device, actor);
    auto geometry = mapper.GetANARIGeometry();

    const std::array<ANARIDataType, 4> expectedTypes{
      ANARI_FLOAT32, ANARI_FLOAT32_VEC2, ANARI_FLOAT32_VEC3, ANARI_FLOAT32_VEC4
    };
    const std::array<std::string, 4> expectedNames{ "scalar", "vector2", "vector3", "vector4" };
    for (viskores::IdComponent fieldIndex = 0; fieldIndex < 4; ++fieldIndex)
    {
      const auto attributeName = std::string("vertex.attribute") + std::to_string(fieldIndex);
      const auto& attribute = inspection->GetArrayParameter(geometry, attributeName);
      VISKORES_TEST_ASSERT(attribute.ElementType ==
                             expectedTypes[static_cast<std::size_t>(fieldIndex)],
                           "A point field was bound with the wrong ANARI element type.");
      VISKORES_TEST_ASSERT(attribute.NumberOfItems == 2,
                           "A point field was bound with the wrong value count.");
      const auto usdName =
        std::string("usd::attribute") + std::to_string(fieldIndex) + std::string(".name");
      VISKORES_TEST_ASSERT(inspection->GetStringParameter(geometry, usdName) ==
                             expectedNames[static_cast<std::size_t>(fieldIndex)],
                           "A point field did not retain its USD attribute name.");
    }
    VISKORES_TEST_ASSERT(
      ReadValues<viskores::Float32>(inspection->GetArrayParameter(geometry, "vertex.attribute0")) ==
        std::vector<viskores::Float32>{ 1.5f, 2.5f },
      "Scalar point values were not converted to Float32.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::Vec3f_32>(
                           inspection->GetArrayParameter(geometry, "vertex.attribute2")) ==
                           std::vector<viskores::Vec3f_32>{ { 5.f, 6.f, 7.f }, { 8.f, 9.f, 10.f } },
                         "Vector point values were not converted to Float32.");
  }
  anari_cpp::release(device, device);
}

void TestInvalidPointAttributesAreRejected()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f },
                                               { 1.f, 0.f, 0.f },
                                               { 2.f, 0.f, 0.f } };
    std::vector<viskores::Float32> cellValues{ 1.f, 2.f, 3.f };
    std::vector<viskores::Float32> wrongLengthValues{ 4.f, 5.f };
    std::vector<viskores::Float32> validValues{ 6.f, 7.f, 8.f };
    std::vector<viskores::Vec<viskores::Float32, 5>> wideValues(3);
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field cellField(
      "cellValues",
      viskores::cont::Field::Association::Cells,
      viskores::cont::make_ArrayHandle(cellValues, viskores::CopyFlag::On));
    viskores::cont::Field wrongLengthField(
      "wrongLength",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(wrongLengthValues, viskores::CopyFlag::On));
    viskores::cont::Field validField(
      "valid",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(validValues, viskores::CopyFlag::On));
    viskores::cont::Field wideField(
      "wide",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(wideValues, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(
      {}, coordinates, cellField, wrongLengthField, validField, wideField);

    viskores::interop::anari::ANARIMapperPoints mapper(device, actor);
    auto geometry = mapper.GetANARIGeometry();

    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.attribute0"),
                         "A cell-associated field was bound as a sphere vertex attribute.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.attribute1"),
                         "A wrong-length point field was bound as a sphere vertex attribute.");
    VISKORES_TEST_ASSERT(inspection->HasParameter(geometry, "vertex.attribute2"),
                         "A valid point field was rejected.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.attribute3"),
                         "A field with more than four components was bound.");
  }
  anari_cpp::release(device, device);
}

void TestInvalidPrimaryFieldDoesNotSelectAnAttribute()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f } };
    std::vector<viskores::Float32> values{ 1.f, 2.f };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor({}, coordinates, field);
    actor.SetPrimaryFieldIndex(-1);

    viskores::interop::anari::ANARIMapperPoints mapper(device, actor);
    auto surface = mapper.GetANARISurface();
    auto material = inspection->GetObjectParameter(surface, "material");

    VISKORES_TEST_ASSERT(!inspection->HasObjectParameter(material, "color"),
                         "An invalid primary field selected unrelated point data.");
  }
  anari_cpp::release(device, device);
}

void TestDegenerateValueRangeProducesFiniteTransform()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f } };
    std::vector<viskores::Float32> values{ 2.f };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperPoints mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates, field));
    auto surface = mapper.GetANARISurface();
    auto material = inspection->GetObjectParameter(surface, "material");
    auto geometry = inspection->GetObjectParameter(surface, "geometry");
    VISKORES_TEST_ASSERT(
      test_equal(inspection->GetFloatParameter(geometry, "radius"), viskores::Float32{ 0.01f }),
      "A one-point data set did not receive the documented fallback radius.");

    mapper.SetANARIColorMapValueRange(viskores::Vec2f_32(2.f, 2.f));
    auto sampler = inspection->GetObjectParameter(material, "color");
    const auto& transform = inspection->GetValueParameter(sampler, "inTransform");
    VISKORES_TEST_ASSERT(transform.Type == ANARI_FLOAT32_MAT4,
                         "The color-map input transform had the wrong ANARI type.");
    const auto transformValues = ReadValues<viskores::Float32>(transform);
    VISKORES_TEST_ASSERT(transformValues.size() == 16,
                         "The color-map input transform was not a 4x4 matrix.");
    for (const auto value : transformValues)
    {
      VISKORES_TEST_ASSERT(viskores::IsFinite(value),
                           "A degenerate value range produced a non-finite transform.");
    }
  }
  anari_cpp::release(device, device);
}

void TestFloat64CoordinatesAreConvertedOnce()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_64> positions{ { 1., 2., 3. }, { 4., 5., 6. } };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::Off));
    viskores::interop::anari::ANARIMapperPoints mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates));
    auto geometry = mapper.GetANARIGeometry();

    const auto& positionParameter = inspection->GetArrayParameter(geometry, "vertex.position");
    VISKORES_TEST_ASSERT(positionParameter.ElementType == ANARI_FLOAT32_VEC3,
                         "Float64 coordinates were not converted to ANARI_FLOAT32_VEC3.");
    VISKORES_TEST_ASSERT(positionParameter.SourceMemory != positions.data(),
                         "Float64 coordinates were incorrectly rebound without conversion.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::Vec3f_32>(positionParameter) ==
                           std::vector<viskores::Vec3f_32>{ { 1.f, 2.f, 3.f }, { 4.f, 5.f, 6.f } },
                         "Float64 coordinate values were not converted correctly.");
  }
  anari_cpp::release(device, device);
}

void TestRepresentativePointSetCreatesOnlyPositionArray()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    constexpr viskores::Id numberOfPoints = 65536;
    std::vector<viskores::Vec3f_32> positions(static_cast<std::size_t>(numberOfPoints));
    for (viskores::Id pointIndex = 0; pointIndex < numberOfPoints; ++pointIndex)
    {
      positions[static_cast<std::size_t>(pointIndex)] =
        viskores::Vec3f_32(static_cast<viskores::Float32>(pointIndex), 0.f, 0.f);
    }
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::Off));
    const auto loweringStart = std::chrono::steady_clock::now();
    viskores::interop::anari::ANARIMapperPoints mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates));
    auto geometry = mapper.GetANARIGeometry();
    const auto loweringEnd = std::chrono::steady_clock::now();
    const auto loweringMilliseconds =
      std::chrono::duration<viskores::Float64, std::milli>(loweringEnd - loweringStart).count();

    VISKORES_TEST_ASSERT(inspection->GetArrayParameter(geometry, "vertex.position").NumberOfItems ==
                           numberOfPoints,
                         "The representative point set did not retain one position per point.");
    VISKORES_TEST_ASSERT(inspection->GetNumberOfArraysCreated() == 1,
                         "The representative point set created identity or radius arrays.");
    VISKORES_TEST_ASSERT(
      inspection->GetNumberOfArrayBytesCreated() ==
        static_cast<std::size_t>(numberOfPoints) * sizeof(viskores::Vec3f_32),
      "The representative point set created more ANARI array storage than its positions.");

    std::cout << "<DartMeasurement name=\"ANARI.PointLoweringMilliseconds\" "
                 "type=\"numeric/double\">"
              << loweringMilliseconds << "</DartMeasurement>\n";
    std::cout << "<DartMeasurement name=\"ANARI.PointArrayBytes\" type=\"numeric/integer\">"
              << inspection->GetNumberOfArrayBytesCreated() << "</DartMeasurement>\n";
    std::cout << "<DartMeasurement name=\"ANARI.PointArrayCount\" type=\"numeric/integer\">"
              << inspection->GetNumberOfArraysCreated() << "</DartMeasurement>\n";
  }
  anari_cpp::release(device, device);
}

void RunTests()
{
  TestContiguousPositionsAndGlobalRadius();
  TestEmptyActorClearsGeometry();
  TestPointAttributesPreserveTypesValuesAndNames();
  TestInvalidPointAttributesAreRejected();
  TestInvalidPrimaryFieldDoesNotSelectAnAttribute();
  TestDegenerateValueRangeProducesFiniteTransform();
  TestFloat64CoordinatesAreConvertedOnce();
  TestRepresentativePointSetCreatesOnlyPositionArray();
}

} // namespace

int UnitTestANARIMapperPointsData(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
