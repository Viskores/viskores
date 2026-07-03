//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>
#include <viskores/rendering/anari-device/ViskoresDevice.h>
#include <viskores/rendering/anari-device/ViskoresDeviceGlobalState.h>

#include <anari/frontend/type_utility.h>

#include <cstring>
#include <limits>
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
    if (appMemory && numberOfItems < 1024 * 1024)
    {
      record.Values.resize(anari::sizeOf(elementType) * numberOfItems);
      std::memcpy(record.Values.data(), appMemory, record.Values.size());
    }
    this->Arrays[array] = std::move(record);
    return array;
  }

  ANARISpatialField newSpatialField(const char* subtype) override
  {
    ++this->SpatialFieldsCreated;
    return this->ViskoresDevice::newSpatialField(subtype);
  }

  const char** getObjectSubtypes(ANARIDataType objectType) override
  {
    if (objectType == ANARI_SPATIAL_FIELD)
    {
      static const char* subtypes[] = { "structuredRegular", "unstructured", nullptr };
      return subtypes;
    }
    return this->ViskoresDevice::getObjectSubtypes(objectType);
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
      auto arrayRecord = this->Arrays.find(array);
      if (arrayRecord != this->Arrays.end())
      {
        this->Parameters[object][name] = arrayRecord->second;
      }
    }
    this->helium::BaseDevice::setParameter(object, name, type, value);
  }

  void unsetParameter(ANARIObject object, const char* name) override
  {
    this->ParameterNames[object].erase(name);
    this->ObjectParameters[object].erase(name);
    this->Parameters[object].erase(name);
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

  viskores::Id GetNumberOfLiveArrays() const
  {
    auto* state = static_cast<viskores_device::ViskoresDeviceGlobalState*>(this->m_state.get());
    return static_cast<viskores::Id>(state->objectCounts.arrays.load());
  }

  void Flush() { this->m_state->commitBuffer.flush(); }

  void ClearPendingCommits() { this->m_state->commitBuffer.clear(); }

  viskores::Id SpatialFieldsCreated{ 0 };

protected:
  int deviceGetProperty(const char* name,
                        ANARIDataType type,
                        void* memory,
                        uint64_t size,
                        uint32_t mask) override
  {
    if (std::strcmp(name, "extension") == 0 && type == ANARI_STRING_LIST)
    {
      static const char* extensions[] = { "ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR",
                                          "ANARI_KHR_SPATIAL_FIELD_UNSTRUCTURED",
                                          "ANARI_KHR_VOLUME_TRANSFER_FUNCTION1D",
                                          nullptr };
      *static_cast<const char***>(memory) = extensions;
      return 1;
    }
    return this->ViskoresDevice::deviceGetProperty(name, type, memory, size, mask);
  }

private:
  std::map<ANARIArray1D, ArrayRecord> Arrays;
  std::map<ANARIObject, std::map<std::string, ArrayRecord>> Parameters;
  std::map<ANARIObject, std::map<std::string, ANARIObject>> ObjectParameters;
  std::map<ANARIObject, std::set<std::string>> ParameterNames;
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
  VISKORES_TEST_ASSERT(rejected, "Expected unstructured volume input to be rejected.");
}

viskores::interop::anari::ANARIActor MakeSingleCellActor(viskores::UInt8 shape,
                                                         viskores::IdComponent pointsPerCell)
{
  std::vector<viskores::Vec3f_32> positions(static_cast<std::size_t>(pointsPerCell));
  std::vector<viskores::Id> connectivity(static_cast<std::size_t>(pointsPerCell));
  std::vector<viskores::Float32> values(static_cast<std::size_t>(pointsPerCell));
  for (viskores::IdComponent pointIndex = 0; pointIndex < pointsPerCell; ++pointIndex)
  {
    positions[static_cast<std::size_t>(pointIndex)] =
      viskores::Vec3f_32(static_cast<viskores::Float32>(pointIndex), 0.f, 0.f);
    connectivity[static_cast<std::size_t>(pointIndex)] = pointIndex;
    values[static_cast<std::size_t>(pointIndex)] = static_cast<viskores::Float32>(pointIndex);
  }

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(pointsPerCell,
               shape,
               pointsPerCell,
               viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On));
  viskores::cont::CoordinateSystem coordinates(
    "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  return viskores::interop::anari::ANARIActor(cellSet, coordinates, field);
}

viskores::interop::anari::ANARIActor MakeStructuredActor()
{
  auto dataSet = viskores::cont::DataSetBuilderUniform::Create(viskores::Id3{ 2, 2, 2 });
  std::vector<viskores::Float32> values(8, 1.f);
  viskores::cont::Field field("values",
                              viskores::cont::Field::Association::Points,
                              viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
  return viskores::interop::anari::ANARIActor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), field);
}

void TestTetrahedronUsesKHRParameterTypes()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    auto spatialField = mapper.GetANARISpatialField();

    const auto& index = inspection->GetArrayParameter(spatialField, "index");
    VISKORES_TEST_ASSERT(index.ElementType == ANARI_UINT32,
                         "Unstructured connectivity was not ANARI_UINT32.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt32>(index) ==
                           std::vector<viskores::UInt32>{ 0, 1, 2, 3 },
                         "Unstructured connectivity was not preserved.");

    const auto& cellIndex = inspection->GetArrayParameter(spatialField, "cell.index");
    VISKORES_TEST_ASSERT(cellIndex.ElementType == ANARI_UINT32,
                         "Unstructured cell offsets were not ANARI_UINT32.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt32>(cellIndex) ==
                           std::vector<viskores::UInt32>{ 0 },
                         "Unstructured cell offsets were not preserved.");

    const auto& cellType = inspection->GetArrayParameter(spatialField, "cell.type");
    VISKORES_TEST_ASSERT(cellType.ElementType == ANARI_UINT8,
                         "Unstructured cell types were not ANARI_UINT8.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt8>(cellType) ==
                           std::vector<viskores::UInt8>{ viskores::CELL_SHAPE_TETRA },
                         "The tetrahedron VTK cell ID was not preserved.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(spatialField, "indexPrefixed"),
                         "The mapper set a parameter outside the current KHR contract.");
  }

  inspection->Flush();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                       "Unstructured mapper arrays outlived the mapper.");
  anari_cpp::release(device, device);
}

void TestHexahedronPreservesVTKCellID()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_HEXAHEDRON, 8);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    auto spatialField = mapper.GetANARISpatialField();
    const auto& cellType = inspection->GetArrayParameter(spatialField, "cell.type");
    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt8>(cellType) ==
                           std::vector<viskores::UInt8>{ viskores::CELL_SHAPE_HEXAHEDRON },
                         "The hexahedron VTK cell ID was not preserved.");
  }
  inspection->Flush();
  anari_cpp::release(device, device);
}

void TestWedgePreservesVTKCellID()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_WEDGE, 6);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    auto spatialField = mapper.GetANARISpatialField();
    const auto& cellType = inspection->GetArrayParameter(spatialField, "cell.type");
    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt8>(cellType) ==
                           std::vector<viskores::UInt8>{ viskores::CELL_SHAPE_WEDGE },
                         "The wedge VTK cell ID was not preserved.");
  }
  inspection->Flush();
  anari_cpp::release(device, device);
}

void TestPyramidPreservesVTKCellID()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_PYRAMID, 5);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    auto spatialField = mapper.GetANARISpatialField();
    const auto& cellType = inspection->GetArrayParameter(spatialField, "cell.type");
    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt8>(cellType) ==
                           std::vector<viskores::UInt8>{ viskores::CELL_SHAPE_PYRAMID },
                         "The pyramid VTK cell ID was not preserved.");
  }
  inspection->Flush();
  anari_cpp::release(device, device);
}

void TestMixedExplicitCellsPreserveTopology()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TETRA,
                                         viskores::CELL_SHAPE_HEXAHEDRON,
                                         viskores::CELL_SHAPE_WEDGE,
                                         viskores::CELL_SHAPE_PYRAMID };
    std::vector<viskores::Id> connectivity(23);
    std::vector<viskores::Vec3f_32> positions(23);
    std::vector<viskores::Float32> values(23);
    for (viskores::Id pointIndex = 0; pointIndex < 23; ++pointIndex)
    {
      connectivity[static_cast<std::size_t>(pointIndex)] = pointIndex;
      positions[static_cast<std::size_t>(pointIndex)] =
        viskores::Vec3f_32(static_cast<viskores::Float32>(pointIndex), 0.f, 0.f);
      values[static_cast<std::size_t>(pointIndex)] = static_cast<viskores::Float32>(pointIndex);
    }
    std::vector<viskores::Id> offsets{ 0, 4, 12, 18, 23 };

    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(23,
                 viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));
    auto spatialField = mapper.GetANARISpatialField();

    VISKORES_TEST_ASSERT(ReadValues<viskores::UInt8>(
                           inspection->GetArrayParameter(spatialField, "cell.type")) == shapes,
                         "Mixed explicit cell types were not preserved.");
    VISKORES_TEST_ASSERT(
      ReadValues<viskores::UInt32>(inspection->GetArrayParameter(spatialField, "cell.index")) ==
        std::vector<viskores::UInt32>{ 0, 4, 12, 18 },
      "Mixed explicit cell offsets were not preserved.");
  }
  inspection->Flush();
  anari_cpp::release(device, device);
}

void TestUnsupportedShapeRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_TRIANGLE, 3);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    ExpectBadValue([&] { mapper.GetANARISpatialField(); },
                   "tetrahedron, hexahedron, wedge, or pyramid");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "An unsupported shape created a partial ANARI spatial field.");
    VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                         "An unsupported shape created partial ANARI arrays.");
  }
  anari_cpp::release(device, device);
}

void TestUnsupportedCellSetRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(viskores::Id2{ 2, 2 });
    std::vector<viskores::Vec3f_32> positions{
      { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f, 0.f }
    };
    std::vector<viskores::Float32> values(4, 1.f);
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));

    ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "CellSetSingleType or CellSetExplicit");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "An unsupported cell set created a partial ANARI spatial field.");
    VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                         "An unsupported cell set created partial ANARI arrays.");
  }
  anari_cpp::release(device, device);
}

void TestOversizedConnectivityRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    const viskores::Id oversizedIndex =
      static_cast<viskores::Id>((std::numeric_limits<viskores::UInt32>::max)()) + 1;
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TETRA };
    std::vector<viskores::Id> connectivity{ 0, 1, 2, oversizedIndex };
    std::vector<viskores::Id> offsets{ 0, 4 };
    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(oversizedIndex + 1,
                 viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));

    std::vector<viskores::Vec3f_32> positions(4);
    std::vector<viskores::Float32> values(4, 1.f);
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));

    ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "connectivity exceeds UInt32");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "Oversized connectivity created a partial ANARI spatial field.");
    VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                         "Oversized connectivity created partial ANARI arrays.");
  }
  anari_cpp::release(device, device);
}

void TestOversizedCellOffsetRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    const viskores::Id oversizedOffset =
      static_cast<viskores::Id>((std::numeric_limits<viskores::UInt32>::max)()) + 1;
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TETRA };
    std::vector<viskores::Id> connectivity{ 0, 1, 2, 3 };
    std::vector<viskores::Id> offsets{ 0, oversizedOffset };
    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(4,
                 viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));

    std::vector<viskores::Vec3f_32> positions(4);
    std::vector<viskores::Float32> values(4, 1.f);
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));

    ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "cell offsets exceeds UInt32");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "Oversized cell offsets created a partial ANARI spatial field.");
    VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                         "Oversized cell offsets created partial ANARI arrays.");
  }
  anari_cpp::release(device, device);
}

void TestMalformedCellOffsetsRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TETRA };
    std::vector<viskores::Id> connectivity{ 0, 1, 2, 3 };
    std::vector<viskores::Id> offsets{ 0, 3 };
    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(4,
                 viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));
    std::vector<viskores::Vec3f_32> positions(4);
    std::vector<viskores::Float32> values(4, 1.f);
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));

    ExpectBadValue([&] { mapper.GetANARISpatialField(); },
                   "cell offsets do not match the cell types");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "Malformed cell offsets created a partial ANARI spatial field.");
  }
  anari_cpp::release(device, device);
}

void TestOutOfRangeConnectivityRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::UInt8> shapes{ viskores::CELL_SHAPE_TETRA };
    std::vector<viskores::Id> connectivity{ 0, 1, 2, 4 };
    std::vector<viskores::Id> offsets{ 0, 4 };
    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(4,
                 viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On),
                 viskores::cont::make_ArrayHandle(offsets, viskores::CopyFlag::On));
    std::vector<viskores::Vec3f_32> positions(4);
    std::vector<viskores::Float32> values(4, 1.f);
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));

    ExpectBadValue([&] { mapper.GetANARISpatialField(); },
                   "connectivity references a point outside the cell set");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "Out-of-range connectivity created a partial ANARI spatial field.");
  }
  anari_cpp::release(device, device);
}

void TestCellDataUsesCellParameter()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Id> connectivity{ 0, 1, 2, 3 };
    viskores::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(4,
                 viskores::CELL_SHAPE_TETRA,
                 4,
                 viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On));
    std::vector<viskores::Vec3f_32> positions(4);
    std::vector<viskores::Float32> values{ 7.f };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Cells,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, viskores::interop::anari::ANARIActor(cellSet, coordinates, field));
    auto spatialField = mapper.GetANARISpatialField();

    const auto& cellData = inspection->GetArrayParameter(spatialField, "cell.data");
    VISKORES_TEST_ASSERT(cellData.ElementType == ANARI_FLOAT32,
                         "Cell data was not lowered as ANARI_FLOAT32.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::Float32>(cellData) ==
                           std::vector<viskores::Float32>{ 7.f },
                         "Cell data values were not preserved.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(spatialField, "vertex.data"),
                         "Cell-associated data was also set as vertex.data.");
  }
  inspection->Flush();
  anari_cpp::release(device, device);
}

void TestVertexDataUsesVertexParameter()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    auto spatialField = mapper.GetANARISpatialField();

    const auto& positions = inspection->GetArrayParameter(spatialField, "vertex.position");
    VISKORES_TEST_ASSERT(positions.ElementType == ANARI_FLOAT32_VEC3,
                         "Vertex positions were not lowered as ANARI_FLOAT32_VEC3.");
    const auto& vertexData = inspection->GetArrayParameter(spatialField, "vertex.data");
    VISKORES_TEST_ASSERT(vertexData.ElementType == ANARI_FLOAT32,
                         "Vertex data was not lowered as ANARI_FLOAT32.");
    VISKORES_TEST_ASSERT(ReadValues<viskores::Float32>(vertexData) ==
                           std::vector<viskores::Float32>{ 0.f, 1.f, 2.f, 3.f },
                         "Vertex data values were not preserved.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(spatialField, "cell.data"),
                         "Point-associated data was also set as cell.data.");
  }
  inspection->Flush();
  anari_cpp::release(device, device);
}

void TestUnsupportedFieldAssociationRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto pointActor = MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4);
    std::vector<viskores::Float32> values{ 1.f };
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::WholeDataSet,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(
      pointActor.GetCellSet(), pointActor.GetCoordinateSystem(), field);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

    ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "point- or cell-associated");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "An unsupported field association created a partial ANARI field.");
  }
  anari_cpp::release(device, device);
}

void TestIncorrectCellDataCardinalityRejectedBeforeANARIObjectCreation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto pointActor = MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4);
    std::vector<viskores::Float32> values{ 1.f, 2.f };
    viskores::cont::Field field("values",
                                viskores::cont::Field::Association::Cells,
                                viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIActor actor(
      pointActor.GetCellSet(), pointActor.GetCoordinateSystem(), field);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);

    ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "one value per cell");
    VISKORES_TEST_ASSERT(inspection->SpatialFieldsCreated == 0,
                         "Incorrect cell data cardinality created a partial ANARI field.");
  }
  anari_cpp::release(device, device);
}

void TestUnstructuredExtensionRequired()
{
  auto* implementation = new viskores_device::ViskoresDevice(nullptr, nullptr);
  auto device = reinterpret_cast<anari_cpp::Device>(implementation);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    ExpectBadValue([&] { mapper.GetANARISpatialField(); }, "ANARI_KHR_SPATIAL_FIELD_UNSTRUCTURED");
  }
  anari_cpp::release(device, device);
}

void TestRepeatedUpdatesReleasePreviousRepresentation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    auto actor = MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4);
    viskores::interop::anari::ANARIMapperVolume mapper(device, actor);
    auto previousField = mapper.GetANARISpatialField();
    inspection->Flush();
    VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 5,
                         "The first unstructured representation did not own five arrays.");

    const std::vector<std::pair<viskores::UInt8, viskores::IdComponent>> replacements{
      { viskores::CELL_SHAPE_HEXAHEDRON, 8 },
      { viskores::CELL_SHAPE_WEDGE, 6 },
      { viskores::CELL_SHAPE_PYRAMID, 5 }
    };
    for (const auto& replacement : replacements)
    {
      mapper.SetActor(MakeSingleCellActor(replacement.first, replacement.second));
      auto nextField = mapper.GetANARISpatialField();
      VISKORES_TEST_ASSERT(nextField != previousField,
                           "An update reused a partially mutated spatial field.");
      previousField = nextField;
      inspection->Flush();
      VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 5,
                           "An update retained arrays from the previous representation.");
    }
  }
  inspection->Flush();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                       "Repeated-update arrays outlived the mapper.");
  anari_cpp::release(device, device);
}

void TestStructuredToUnstructuredTransitionIsAtomic()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::interop::anari::ANARIMapperVolume mapper(device, MakeStructuredActor());
    auto originalField = mapper.GetANARISpatialField();
    auto volume = mapper.GetANARIVolume();

    mapper.SetActor(MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4));
    auto replacementField = mapper.GetANARISpatialField();
    VISKORES_TEST_ASSERT(replacementField != originalField,
                         "The structured spatial field was mutated in place.");
    VISKORES_TEST_ASSERT(mapper.GetANARIVolume() == volume,
                         "Changing representation replaced the ANARI volume.");
    VISKORES_TEST_ASSERT(inspection->GetObjectParameter(volume, "value") == replacementField,
                         "The volume did not atomically switch to the candidate spatial field.");
    VISKORES_TEST_ASSERT(!inspection->HasParameter(replacementField, "data") &&
                           !inspection->HasParameter(replacementField, "origin") &&
                           !inspection->HasParameter(replacementField, "spacing"),
                         "The unstructured field retained structured parameters.");
    VISKORES_TEST_ASSERT(inspection->HasParameter(replacementField, "vertex.position") &&
                           inspection->HasParameter(replacementField, "index") &&
                           inspection->HasParameter(replacementField, "cell.index") &&
                           inspection->HasParameter(replacementField, "cell.type"),
                         "The replacement field was incomplete.");
  }
  inspection->ClearPendingCommits();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                       "Transition arrays outlived the mapper.");
  anari_cpp::release(device, device);
}

void TestRejectedUpdatePreservesPreviousRepresentation()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::interop::anari::ANARIMapperVolume mapper(
      device, MakeSingleCellActor(viskores::CELL_SHAPE_TETRA, 4));
    auto originalField = mapper.GetANARISpatialField();
    inspection->Flush();

    ExpectBadValue([&] { mapper.SetActor(MakeSingleCellActor(viskores::CELL_SHAPE_TRIANGLE, 3)); },
                   "tetrahedron, hexahedron, wedge, or pyramid");
    VISKORES_TEST_ASSERT(mapper.GetANARISpatialField() == originalField,
                         "A rejected update replaced the current spatial field.");
    inspection->Flush();
    VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 5,
                         "A rejected update damaged the previous representation.");
  }
  inspection->Flush();
  VISKORES_TEST_ASSERT(inspection->GetNumberOfLiveArrays() == 0,
                       "Rejected-update arrays outlived the mapper.");
  anari_cpp::release(device, device);
}

void RunTests()
{
  TestTetrahedronUsesKHRParameterTypes();
  TestHexahedronPreservesVTKCellID();
  TestWedgePreservesVTKCellID();
  TestPyramidPreservesVTKCellID();
  TestMixedExplicitCellsPreserveTopology();
  TestUnsupportedShapeRejectedBeforeANARIObjectCreation();
  TestUnsupportedCellSetRejectedBeforeANARIObjectCreation();
  TestOversizedConnectivityRejectedBeforeANARIObjectCreation();
  TestOversizedCellOffsetRejectedBeforeANARIObjectCreation();
  TestMalformedCellOffsetsRejectedBeforeANARIObjectCreation();
  TestOutOfRangeConnectivityRejectedBeforeANARIObjectCreation();
  TestCellDataUsesCellParameter();
  TestVertexDataUsesVertexParameter();
  TestUnsupportedFieldAssociationRejectedBeforeANARIObjectCreation();
  TestIncorrectCellDataCardinalityRejectedBeforeANARIObjectCreation();
  TestUnstructuredExtensionRequired();
  TestRepeatedUpdatesReleasePreviousRepresentation();
  TestStructuredToUnstructuredTransitionIsAtomic();
  TestRejectedUpdatePreservesPreviousRepresentation();
}

} // namespace

int UnitTestANARIUnstructuredVolume(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
