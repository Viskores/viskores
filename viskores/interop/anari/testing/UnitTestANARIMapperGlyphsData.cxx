//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/Math.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
#include <viskores/rendering/anari-device/ViskoresDevice.h>

#include <anari/frontend/type_utility.h>

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

class InspectionDevice : public viskores_device::ViskoresDevice
{
public:
  explicit InspectionDevice(bool supportsCones = true)
    : viskores_device::ViskoresDevice(nullptr, nullptr)
    , SupportsCones(supportsCones)
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
    ++this->NumberOfObjectsCreated;
    this->Arrays[array] = std::move(record);
    return array;
  }

  ANARIGeometry newGeometry(const char* type) override
  {
    ++this->NumberOfObjectsCreated;
    return this->ViskoresDevice::newGeometry(type);
  }

  ANARIMaterial newMaterial(const char* type) override
  {
    ++this->NumberOfObjectsCreated;
    return this->ViskoresDevice::newMaterial(type);
  }

  ANARISurface newSurface() override
  {
    ++this->NumberOfObjectsCreated;
    return this->ViskoresDevice::newSurface();
  }

  void setParameter(ANARIObject object,
                    const char* name,
                    ANARIDataType type,
                    const void* value) override
  {
    if (type == ANARI_ARRAY1D && value)
    {
      auto array = *static_cast<ANARIArray1D const*>(value);
      auto record = this->Arrays.find(array);
      if (record != this->Arrays.end())
      {
        this->ArrayParameters[object][name] = record->second;
      }
    }
    this->helium::BaseDevice::setParameter(object, name, type, value);
  }

  void unsetParameter(ANARIObject object, const char* name) override
  {
    this->ArrayParameters[object].erase(name);
    this->helium::BaseDevice::unsetParameter(object, name);
  }

  const char** getObjectSubtypes(ANARIDataType objectType) override
  {
    if (objectType == ANARI_GEOMETRY)
    {
      static const char* supportedSubtypes[] = { "cone", nullptr };
      static const char* unsupportedSubtypes[] = { "sphere", nullptr };
      return this->SupportsCones ? supportedSubtypes : unsupportedSubtypes;
    }
    return this->ViskoresDevice::getObjectSubtypes(objectType);
  }

  int getProperty(ANARIObject object,
                  const char* name,
                  ANARIDataType type,
                  void* memory,
                  uint64_t size,
                  uint32_t mask) override
  {
    if (std::strcmp(name, "extension") == 0 && type == ANARI_STRING_LIST &&
        size >= sizeof(const char**))
    {
      static const char* supportedExtensions[] = { "ANARI_KHR_GEOMETRY_CONE", nullptr };
      static const char* unsupportedExtensions[] = { nullptr };
      const char** extensions = this->SupportsCones ? supportedExtensions : unsupportedExtensions;
      std::memcpy(memory, &extensions, sizeof(extensions));
      return 1;
    }
    return this->ViskoresDevice::getProperty(object, name, type, memory, size, mask);
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

  std::size_t GetNumberOfObjectsCreated() const { return this->NumberOfObjectsCreated; }

private:
  bool SupportsCones{ true };
  std::map<ANARIArray1D, ArrayRecord> Arrays;
  std::map<ANARIObject, std::map<std::string, ArrayRecord>> ArrayParameters;
  std::size_t NumberOfObjectsCreated{ 0 };
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

void TestPointVectorsHandleZeroVectorsAndDegenerateBounds()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions(3, viskores::Vec3f_32{ 2.f, 3.f, 4.f });
    std::vector<viskores::Vec3f_32> vectors{ { 0.f, 0.f, 0.f },
                                             { 2.f, 0.f, 0.f },
                                             { 200.f, 0.f, 0.f } };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("vectors",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(vectors, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperGlyphs mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates, field));

    auto geometry = mapper.GetANARIGeometry();
    const auto positionsOutput =
      ReadValues<viskores::Vec3f_32>(inspection->GetArrayParameter(geometry, "vertex.position"));
    const auto radiiOutput =
      ReadValues<viskores::Float32>(inspection->GetArrayParameter(geometry, "vertex.radius"));

    VISKORES_TEST_ASSERT(positionsOutput.size() == 12,
                         "Point vectors did not generate four cone vertices per sample.");
    VISKORES_TEST_ASSERT(radiiOutput.size() == 12,
                         "Point vectors did not generate four cone radii per sample.");
    for (std::size_t index = 0; index < positionsOutput.size(); ++index)
    {
      for (viskores::IdComponent component = 0; component < 3; ++component)
      {
        VISKORES_TEST_ASSERT(viskores::IsFinite(positionsOutput[index][component]),
                             "Glyph generation produced a non-finite position.");
      }
      VISKORES_TEST_ASSERT(viskores::IsFinite(radiiOutput[index]),
                           "Glyph generation produced a non-finite radius.");
    }
    for (std::size_t index = 0; index < 4; ++index)
    {
      VISKORES_TEST_ASSERT(test_equal(positionsOutput[index], positions[0]),
                           "A zero vector did not remain at its sample position.");
      VISKORES_TEST_ASSERT(test_equal(radiiOutput[index], 0.f),
                           "A zero vector produced visible cone geometry.");
    }
    VISKORES_TEST_ASSERT(radiiOutput[4] > 0.f,
                         "Degenerate coordinate bounds produced a zero glyph size.");
    for (std::size_t vertex = 0; vertex < 4; ++vertex)
    {
      VISKORES_TEST_ASSERT(test_equal(positionsOutput[4 + vertex], positionsOutput[8 + vertex]),
                           "Vector magnitude unexpectedly changed glyph length.");
      VISKORES_TEST_ASSERT(test_equal(radiiOutput[4 + vertex], radiiOutput[8 + vertex]),
                           "Vector magnitude unexpectedly changed glyph radius.");
    }
  }
  anari_cpp::release(device, device);
}

void TestCellVectorsGenerateGlyphsAtCellCenters()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::cont::DataSetBuilderUniform builder;
    auto dataSet = builder.Create(viskores::Id2{ 3, 2 });
    std::vector<viskores::Vec3f_32> vectors(
      static_cast<std::size_t>(dataSet.GetCellSet().GetNumberOfCells()),
      viskores::Vec3f_32{ 0.f, 1.f, 0.f });
    dataSet.AddCellField("vectors",
                         viskores::cont::make_ArrayHandle(vectors, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperGlyphs mapper(
      device, viskores::interop::anari::ANARIActor(dataSet, "vectors"));

    auto geometry = mapper.GetANARIGeometry();
    const auto& positionsParameter = inspection->GetArrayParameter(geometry, "vertex.position");
    const auto& radiiOutput = inspection->GetArrayParameter(geometry, "vertex.radius");
    const auto positionsOutput = ReadValues<viskores::Vec3f_32>(positionsParameter);
    const auto expectedVertices =
      static_cast<viskores::UInt64>(dataSet.GetCellSet().GetNumberOfCells() * 4);

    VISKORES_TEST_ASSERT(positionsParameter.NumberOfItems == expectedVertices,
                         "Cell vectors did not generate four cone vertices per cell.");
    VISKORES_TEST_ASSERT(radiiOutput.NumberOfItems == expectedVertices,
                         "Cell vectors did not generate four cone radii per cell.");
    VISKORES_TEST_ASSERT(test_equal(positionsOutput[1], viskores::Vec3f_32{ 0.5f, 0.5f, 0.f }) &&
                           test_equal(positionsOutput[5], viskores::Vec3f_32{ 1.5f, 0.5f, 0.f }),
                         "Cell glyphs were not generated at cell centers.");
  }
  anari_cpp::release(device, device);
}

void ExpectInvalidField(anari_cpp::Device device, const viskores::interop::anari::ANARIActor& actor)
{
  bool threwExpectedError = false;
  try
  {
    viskores::interop::anari::ANARIMapperGlyphs mapper(device, actor);
    mapper.GetANARIGeometry();
  }
  catch (const viskores::cont::ErrorBadValue&)
  {
    threwExpectedError = true;
  }
  VISKORES_TEST_ASSERT(threwExpectedError,
                       "An invalid glyph field was not rejected with ErrorBadValue.");
}

void TestInvalidAssociationsAndFieldLengthsAreRejected()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    viskores::cont::DataSetBuilderUniform builder;
    auto dataSet = builder.Create(viskores::Id2{ 3, 2 });
    const auto& cells = dataSet.GetCellSet();
    const auto& coordinates = dataSet.GetCoordinateSystem();

    std::vector<viskores::Vec3f_32> wholeDataSetVector{ { 1.f, 0.f, 0.f } };
    viskores::cont::Field wholeDataSetField(
      "vectors",
      viskores::cont::Field::Association::WholeDataSet,
      viskores::cont::make_ArrayHandle(wholeDataSetVector, viskores::CopyFlag::On));
    ExpectInvalidField(device,
                       viskores::interop::anari::ANARIActor(cells, coordinates, wholeDataSetField));

    std::vector<viskores::Vec3f_32> shortPointVectors(
      static_cast<std::size_t>(coordinates.GetNumberOfPoints() - 1),
      viskores::Vec3f_32{ 1.f, 0.f, 0.f });
    viskores::cont::Field shortPointField(
      "vectors",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(shortPointVectors, viskores::CopyFlag::On));
    ExpectInvalidField(device,
                       viskores::interop::anari::ANARIActor(cells, coordinates, shortPointField));

    std::vector<viskores::Vec3f_32> longCellVectors(
      static_cast<std::size_t>(cells.GetNumberOfCells() + 1), viskores::Vec3f_32{ 1.f, 0.f, 0.f });
    viskores::cont::Field longCellField(
      "vectors",
      viskores::cont::Field::Association::Cells,
      viskores::cont::make_ArrayHandle(longCellVectors, viskores::CopyFlag::On));
    ExpectInvalidField(device,
                       viskores::interop::anari::ANARIActor(cells, coordinates, longCellField));

    std::vector<viskores::Float32> scalarValues(
      static_cast<std::size_t>(coordinates.GetNumberOfPoints()), 1.f);
    viskores::cont::Field scalarField(
      "scalars",
      viskores::cont::Field::Association::Points,
      viskores::cont::make_ArrayHandle(scalarValues, viskores::CopyFlag::On));
    ExpectInvalidField(device,
                       viskores::interop::anari::ANARIActor(cells, coordinates, scalarField));
  }
  anari_cpp::release(device, device);
}

void TestOffsetChangesRegenerateMaterializedGeometry()
{
  auto* inspection = new InspectionDevice;
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 2.f, 3.f, 4.f } };
    std::vector<viskores::Vec3f_32> vectors{ { 1.f, 0.f, 0.f } };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("vectors",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(vectors, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperGlyphs mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates, field));

    mapper.SetOffsetGlyphs(true);
    auto geometry = mapper.GetANARIGeometry();
    auto offsetPositions =
      ReadValues<viskores::Vec3f_32>(inspection->GetArrayParameter(geometry, "vertex.position"));
    VISKORES_TEST_ASSERT(test_equal(offsetPositions[0], positions[0]),
                         "Offset enabled before materialization was not applied.");

    mapper.SetOffsetGlyphs(false);
    auto centeredPositions =
      ReadValues<viskores::Vec3f_32>(inspection->GetArrayParameter(geometry, "vertex.position"));
    VISKORES_TEST_ASSERT(centeredPositions != offsetPositions,
                         "Changing offset after materialization did not regenerate geometry.");
    VISKORES_TEST_ASSERT(centeredPositions[0][0] > positions[0][0],
                         "Disabling offset did not center the glyph around its sample.");
  }
  anari_cpp::release(device, device);
}

void TestUnsupportedConeDeviceCreatesNoPartialObjects()
{
  auto* inspection = new InspectionDevice(false);
  auto device = reinterpret_cast<anari_cpp::Device>(inspection);
  {
    std::vector<viskores::Vec3f_32> positions{ { 0.f, 0.f, 0.f } };
    std::vector<viskores::Vec3f_32> vectors{ { 1.f, 0.f, 0.f } };
    viskores::cont::CoordinateSystem coordinates(
      "coordinates", viskores::cont::make_ArrayHandle(positions, viskores::CopyFlag::On));
    viskores::cont::Field field("vectors",
                                viskores::cont::Field::Association::Points,
                                viskores::cont::make_ArrayHandle(vectors, viskores::CopyFlag::On));
    viskores::interop::anari::ANARIMapperGlyphs mapper(
      device, viskores::interop::anari::ANARIActor({}, coordinates, field));

    bool threwExpectedError = false;
    try
    {
      mapper.GetANARISurface();
    }
    catch (const viskores::cont::ErrorBadValue& error)
    {
      threwExpectedError =
        std::string(error.GetMessage()).find("ANARI_KHR_GEOMETRY_CONE") != std::string::npos;
    }
    VISKORES_TEST_ASSERT(threwExpectedError,
                         "An unsupported cone device was not reported with ErrorBadValue.");
    VISKORES_TEST_ASSERT(inspection->GetNumberOfObjectsCreated() == 0,
                         "The mapper created partial ANARI objects on an unsupported device.");
  }
  anari_cpp::release(device, device);
}

void RunTests()
{
  TestPointVectorsHandleZeroVectorsAndDegenerateBounds();
  TestCellVectorsGenerateGlyphsAtCellCenters();
  TestInvalidAssociationsAndFieldLengthsAreRejected();
  TestOffsetChangesRegenerateMaterializedGeometry();
  TestUnsupportedConeDeviceCreatesNoPartialObjects();
}

} // namespace

int UnitTestANARIMapperGlyphsData(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
