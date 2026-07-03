//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/rendering/anari-device/ViskoresDevice.h>

#include <anari/frontend/type_utility.h>

#include <cstring>
#include <map>
#include <set>
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
    this->ParameterNames[object].insert(name);
    if (anari::isObject(type) && value)
    {
      this->ObjectParameters[object][name] = *static_cast<ANARIObject const*>(value);
    }
    if (type == ANARI_ARRAY1D && value)
    {
      auto array = *static_cast<ANARIArray1D const*>(value);
      auto record = this->Arrays.find(array);
      if (record != this->Arrays.end())
      {
        this->Parameters[object][name] = record->second;
      }
    }
    if (type == ANARI_STRING && value)
    {
      this->StringParameters[object][name] = static_cast<const char*>(value);
    }
    this->helium::BaseDevice::setParameter(object, name, type, value);
  }

  void unsetParameter(ANARIObject object, const char* name) override
  {
    this->ParameterNames[object].erase(name);
    this->ObjectParameters[object].erase(name);
    this->Parameters[object].erase(name);
    this->StringParameters[object].erase(name);
    this->helium::BaseDevice::unsetParameter(object, name);
  }

  const ArrayRecord& GetArrayParameter(ANARIObject object, const std::string& name) const
  {
    auto objectParameters = this->Parameters.find(object);
    VISKORES_TEST_ASSERT(objectParameters != this->Parameters.end(),
                         "No parameters were recorded for the ANARI object.");
    auto parameter = objectParameters->second.find(name);
    VISKORES_TEST_ASSERT(parameter != objectParameters->second.end(),
                         "The expected ANARI array parameter was not set: " + name);
    return parameter->second;
  }

  bool HasParameter(ANARIObject object, const std::string& name) const
  {
    auto parameterNames = this->ParameterNames.find(object);
    return parameterNames != this->ParameterNames.end() &&
      parameterNames->second.find(name) != parameterNames->second.end();
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

private:
  std::map<ANARIArray1D, ArrayRecord> Arrays;
  std::map<ANARIArray, void*> MappedArrays;
  std::map<ANARIObject, std::map<std::string, ArrayRecord>> Parameters;
  std::map<ANARIObject, std::map<std::string, ANARIObject>> ObjectParameters;
  std::map<ANARIObject, std::set<std::string>> ParameterNames;
  std::map<ANARIObject, std::map<std::string, std::string>> StringParameters;
};

template <typename ValueType>
std::vector<ValueType> ReadValues(const ArrayRecord& record)
{
  VISKORES_TEST_ASSERT(record.Values.size() % sizeof(ValueType) == 0,
                       "Recorded ANARI array has an unexpected size.");
  std::vector<ValueType> values(record.Values.size() / sizeof(ValueType));
  std::memcpy(values.data(), record.Values.data(), record.Values.size());
  return values;
}

viskores::interop::anari::ANARIActor MakeTriangulatedActor()
{
  std::vector<viskores::Vec3f_32> positions{
    { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }, { 2.f, 1.f, 0.f }
  };
  std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_QUAD, viskores::CELL_SHAPE_TRIANGLE };
  std::vector<viskores::Id> connectivity{ 0, 1, 2, 3, 0, 2, 4 };
  std::vector<viskores::Id> offsets{ 0, 4, 7 };
  std::vector<viskores::Float32> pointValues{ 1.f, 2.f, 3.f, 4.f, 5.f };
  std::vector<viskores::Float32> cellValues{ 10.f, 20.f };

  viskores::cont::CellSetExplicit<> cells;
  cells.Fill(5,
             viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
             viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
             viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));
  viskores::cont::CoordinateSystem coordinates(
    "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
  viskores::cont::Field pointField(
    "pointValues",
    viskores::cont::Field::Association::Points,
    viskores::cont::make_ArrayHandle(pointValues, viskores::CopyFlag::On));
  viskores::cont::Field cellField(
    "cellValues",
    viskores::cont::Field::Association::Cells,
    viskores::cont::make_ArrayHandle(cellValues, viskores::CopyFlag::On));
  return viskores::interop::anari::ANARIActor(cells, coordinates, pointField, cellField);
}

void TestIndexedTopologyPreservesSharedVertices()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::interop::anari::ANARIMapperTriangles mapper(device, MakeTriangulatedActor());
    auto geometry = mapper.GetANARIGeometry();

    const auto& positions = inspection->GetArrayParameter(geometry, "vertex.position");
    VISKORES_TEST_ASSERT(positions.ElementType == ANARI_FLOAT32_VEC3,
                         "Triangle positions were not ANARI_FLOAT32_VEC3.");
    VISKORES_TEST_ASSERT(positions.NumberOfItems == 5,
                         "The mapper expanded shared positions per triangle.");

    const auto& indices = inspection->GetArrayParameter(geometry, "primitive.index");
    VISKORES_TEST_ASSERT(indices.ElementType == ANARI_UINT32_VEC3,
                         "Small triangle indices were not ANARI_UINT32_VEC3.");
    VISKORES_TEST_ASSERT(
      ReadValues<viskores::Vec3ui_32>(indices) ==
        std::vector<viskores::Vec3ui_32>{ { 0, 1, 2 }, { 0, 2, 3 }, { 0, 2, 4 } },
      "Triangle connectivity was not preserved.");

    const auto& pointField = inspection->GetArrayParameter(geometry, "vertex.attribute0");
    VISKORES_TEST_ASSERT(pointField.NumberOfItems == 5,
                         "The mapper expanded point-field values per triangle.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::Float32>(pointField) ==
                           std::vector<viskores::Float32>{ 1.f, 2.f, 3.f, 4.f, 5.f },
                         "Point-field values were not preserved.");
  }
  anari_cpp::release(device, device);
}

void TestCellFieldsBecomePrimitiveAttributes()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeTriangulatedActor();
    actor.SetPrimaryFieldIndex(1);
    viskores::interop::anari::ANARIMapperTriangles mapper(device, actor);
    auto geometry = mapper.GetANARIGeometry();

    const auto& cellField = inspection->GetArrayParameter(geometry, "primitive.attribute1");
    VISKORES_TEST_ASSERT(cellField.NumberOfItems == 3,
                         "The mapper did not create one cell-field value per triangle.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::Float32>(cellField) ==
                           std::vector<viskores::Float32>{ 10.f, 10.f, 20.f },
                         "Triangulated primitives did not retain their source-cell values.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.attribute1"),
                         "A cell field was incorrectly bound as a vertex attribute.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "primitive.attribute0"),
                         "A point field was incorrectly bound as a primitive attribute.");

    auto surface = mapper.GetANARISurface();
    auto material = inspection->GetObjectParameter(surface, "material");
    auto sampler = inspection->GetObjectParameter(material, "color");
    VISKORES_TEST_ASSERT(inspection->GetStringParameter(sampler, "inAttribute") == "attribute1",
                         "The material did not select the primary cell field.");
  }
  anari_cpp::release(device, device);
}

void TestUnsupportedFieldsAreIgnored()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto baseActor = MakeTriangulatedActor();
    std::vector<viskores::Float32> wholeValue{ 42.f };
    std::vector<viskores::Vec2f_32> vectorValues(5, { 1.f, 2.f });
    viskores::cont::Field wholeField(
      "wholeValue",
      viskores::cont::Field::Association::WholeDataSet,
      viskores::cont::make_ArrayHandle(wholeValue, viskores::CopyFlag::On));
    viskores::cont::Field vectorField(
      "vectorValues",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(vectorValues, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(baseActor.GetCellSet(),
                                               baseActor.GetCoordinateSystem(),
                                               baseActor.GetField(0),
                                               baseActor.GetField(1),
                                               wholeField,
                                               vectorField);

    viskores::interop::anari::ANARIMapperTriangles mapper(device, actor);
    auto geometry = mapper.GetANARIGeometry();

    for (const auto& parameter : { "vertex.attribute2",
                                   "primitive.attribute2",
                                   "vertex.attribute3",
                                   "primitive.attribute3" })
    {
      VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, parameter),
                           "An unsupported field was bound to triangle geometry.");
    }
  }
  anari_cpp::release(device, device);
}

void TestNormalsCanBeToggledBeforeAndAfterMaterialization()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id2{ 3, 3 });
    std::vector<viskores::Float32> pointValues(9, 1.f);
    viskores::cont::Field pointField(
      "pointValues",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(pointValues, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), pointField);
    viskores::interop::anari::ANARIMapperTriangles mapper(device, actor);
    mapper.SetCalculateNormals(true);
    auto geometry = mapper.GetANARIGeometry();

    const auto& initialNormals = inspection->GetArrayParameter(geometry, "vertex.normal");
    VISKORES_TEST_ASSERT(initialNormals.NumberOfItems == 9,
                         "Normals enabled before materialization were expanded or omitted.");

    mapper.SetCalculateNormals(false);
    VISKORES_TEST_ASSERT(!inspection->HasParameter(geometry, "vertex.normal"),
                         "Disabling normals after materialization left vertex.normal bound.");

    mapper.SetCalculateNormals(true);
    const auto& regeneratedNormals = inspection->GetArrayParameter(geometry, "vertex.normal");
    VISKORES_TEST_ASSERT(regeneratedNormals.NumberOfItems == 9,
                         "Re-enabled normals were not regenerated per shared vertex.");
  }
  anari_cpp::release(device, device);
}

void TestRepresentativeMeshKeepsSharedArraySizes()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    constexpr viskores::Id pointsPerDimension = 257;
    constexpr viskores::Id numberOfPoints = pointsPerDimension * pointsPerDimension;
    constexpr viskores::Id numberOfTriangles = 2 * 256 * 256;
    auto dataSet = viskores::cont::DataSetBuilderUniform::Create(
      viskores::Id2{ pointsPerDimension, pointsPerDimension });
    std::vector<viskores::Float32> pointValues(static_cast<std::size_t>(numberOfPoints), 1.f);
    viskores::cont::Field pointField(
      "pointValues",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(pointValues, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), pointField);

    viskores::interop::anari::ANARIMapperTriangles mapper(device, actor);
    auto geometry = mapper.GetANARIGeometry();

    VISKORES_TEST_ASSERT(inspection->GetArrayParameter(geometry, "vertex.position").NumberOfItems ==
                           numberOfPoints,
                         "Representative-mesh positions were expanded per triangle.");
    VISKORES_TEST_ASSERT(
      inspection->GetArrayParameter(geometry, "vertex.attribute0").NumberOfItems == numberOfPoints,
      "Representative-mesh point values were expanded per triangle.");
    VISKORES_TEST_ASSERT(
      inspection->GetArrayParameter(geometry, "primitive.index").NumberOfItems == numberOfTriangles,
      "Representative-mesh connectivity did not contain one index triple per triangle.");
  }
  anari_cpp::release(device, device);
}

void RunTests()
{
  TestIndexedTopologyPreservesSharedVertices();
  TestCellFieldsBecomePrimitiveAttributes();
  TestUnsupportedFieldsAreIgnored();
  TestNormalsCanBeToggledBeforeAndAfterMaterialization();
  TestRepresentativeMeshKeepsSharedArraySizes();
}

} // namespace

int UnitTestANARIMapperTrianglesData(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
