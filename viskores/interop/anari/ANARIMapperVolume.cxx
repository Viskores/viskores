//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/Logging.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>

#include <limits>

namespace viskores
{
namespace interop
{
namespace anari
{

ANARIMapperVolume::ANARIMapperVolume(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperVolume::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperVolume::~ANARIMapperVolume() = default;

void ANARIMapperVolume::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
}

void ANARIMapperVolume::SetANARIColorMap(anari_cpp::Array1D color,
                                         anari_cpp::Array1D opacity,
                                         bool releaseArrays)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "color", color);
  anari_cpp::setParameter(d, v, "opacity", opacity);
  anari_cpp::commitParameters(d, v);
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperVolume::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "valueRange", ANARI_FLOAT32_BOX1, &valueRange);
  anari_cpp::commitParameters(d, v);
}

void ANARIMapperVolume::SetANARIColorMapOpacityScale(viskores::Float32 opacityScale)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "densityScale", opacityScale);
  anari_cpp::commitParameters(d, v);
}

anari_cpp::SpatialField ANARIMapperVolume::GetANARISpatialField()
{
  if (this->Handles->SpatialField)
    return this->Handles->SpatialField;

  this->ConstructArrays();
  this->UpdateSpatialField();
  return this->Handles->SpatialField;
}

anari_cpp::Volume ANARIMapperVolume::GetANARIVolume()
{
  if (this->Handles->Volume)
    return this->Handles->Volume;

  auto d = this->GetDevice();

  this->Handles->Volume = anari_cpp::newObject<anari_cpp::Volume>(d, "transferFunction1D");

  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
  auto* colors = anari_cpp::map<viskores::Vec3f_32>(d, colorArray);
  colors[0] = viskores::Vec3f_32(1.f, 0.f, 0.f);
  colors[1] = viskores::Vec3f_32(0.f, 1.f, 0.f);
  colors[2] = viskores::Vec3f_32(0.f, 0.f, 1.f);
  anari_cpp::unmap(d, colorArray);

  auto opacityArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* opacities = anari_cpp::map<float>(d, opacityArray);
  opacities[0] = 0.f;
  opacities[1] = 1.f;
  anari_cpp::unmap(d, opacityArray);

  anari_cpp::setAndReleaseParameter(d, this->Handles->Volume, "color", colorArray);
  anari_cpp::setAndReleaseParameter(d, this->Handles->Volume, "opacity", opacityArray);
  // Keep old name as a parameter for bug-backwards compatibility: 'field' isn't the right name
  anari_cpp::setParameter(d, this->Handles->Volume, "field", this->GetANARISpatialField());
  anari_cpp::setParameter(d, this->Handles->Volume, "value", this->GetANARISpatialField());
  anari_cpp::setParameter(d, this->Handles->Volume, "name", this->MakeObjectName("volume"));
  anari_cpp::commitParameters(d, this->Handles->Volume);

  SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));

  return this->Handles->Volume;
}

const UnstructuredVolumeParameters& ANARIMapperVolume::GetANARIUnstructuredParameters() const
{
  return this->Handles->UnstructuredParameters;
}

const StructuredVolumeParameters& ANARIMapperVolume::GetANARIStructuredParameters() const
{
  return this->Handles->StructuredParameters;
}

// ANARI's KHR_SPATIAL_FIELD_UNSTRUCTURED `cell.type` uses the VTK cell IDs:
//    10 = tetrahedron, 12 = hexahedron, 13 = wedge, 14 = pyramid
// Viskores' CELL_SHAPE_* values for these shapes are the same, so they pass through
// unchanged. Every other shape is marked invalid (255).
static_assert(viskores::CELL_SHAPE_TETRA == 10 && viskores::CELL_SHAPE_HEXAHEDRON == 12 &&
                viskores::CELL_SHAPE_WEDGE == 13 && viskores::CELL_SHAPE_PYRAMID == 14,
              "Viskores cell shape IDs must match ANARI cell.type values");

struct ToAnariCellType
{
  VISKORES_EXEC_CONT viskores::UInt8 operator()(viskores::UInt8 shape) const
  {
    switch (shape)
    {
      case viskores::CELL_SHAPE_TETRA:
      case viskores::CELL_SHAPE_HEXAHEDRON:
      case viskores::CELL_SHAPE_WEDGE:
      case viskores::CELL_SHAPE_PYRAMID:
        return shape;
      default:
        return viskores::UInt8(-1);
    }
  }
};

namespace
{

struct UnstructuredVolumeArrays
{
  viskores::cont::ArrayHandle<viskores::Vec3f_32> VertexPosition;
  viskores::cont::ArrayHandle<viskores::Float32> VertexData;
  viskores::cont::ArrayHandle<viskores::UInt32> Index;
  viskores::cont::ArrayHandle<viskores::UInt32> CellIndex;
  viskores::cont::ArrayHandle<viskores::Float32> CellData;
  viskores::cont::ArrayHandle<viskores::UInt8> CellType;
};

// Copy the cell types, connectivity and offsets of an unstructured cell set into the
// arrays sent to ANARI. KHR_SPATIAL_FIELD_UNSTRUCTURED requires UInt32 `index` and
// `cell.index`, so this returns false, and copies nothing, when the connectivity has a
// point index or size that does not fit in UInt32.
template <typename CellSetType>
bool CopyUnstructuredCells(const CellSetType& cellSet, UnstructuredVolumeArrays& arrays)
{
  const viskores::TopologyElementTagCell visit;
  const viskores::TopologyElementTagPoint incident;
  constexpr auto maxIndex = static_cast<viskores::Id>(std::numeric_limits<viskores::UInt32>::max());

  auto connectivity = cellSet.GetConnectivityArray(visit, incident);
  // The offsets sent as `cell.index` are all smaller than the connectivity size.
  if (connectivity.GetNumberOfValues() - 1 > maxIndex ||
      viskores::cont::Algorithm::Reduce(connectivity, viskores::Id(0), viskores::Maximum()) >
        maxIndex)
  {
    return false;
  }

  viskores::cont::ArrayCopyDevice(viskores::cont::make_ArrayHandleTransform(
                                    cellSet.GetShapesArray(visit, incident), ToAnariCellType{}),
                                  arrays.CellType);
  viskores::cont::ArrayCopyDevice(connectivity, arrays.Index);
  viskores::cont::ArrayCopyDevice(cellSet.GetOffsetsArray(visit, incident), arrays.CellIndex);
  return true;
}

} // namespace

void ANARIMapperVolume::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  this->Current = true;
  this->Valid = false;

  auto d = this->GetDevice();

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();
  const auto& cells = actor.GetCellSet();
  const auto& fieldArray = actor.GetField().GetData();

  const bool isPointBased =
    actor.GetField().GetAssociation() == viskores::cont::Field::Association::Points;
  const bool isStructured = cells.CanConvert<viskores::cont::CellSetStructured<3>>();
  const bool isScalar = fieldArray.GetNumberOfComponentsFlat() == 1;

  this->Handles->ReleaseArrays();
  anari_cpp::release(d, this->Handles->SpatialField);
  this->Handles->SpatialField = nullptr;

  // Reject inputs neither branch below can map, before creating any ANARI object.
  const char* unsupportedReason = nullptr;
  UnstructuredVolumeArrays unstructuredArrays;
  if (cells.GetNumberOfCells() == 0)
  {
    unsupportedReason = "the cell set is empty";
  }
  else if (!isScalar)
  {
    unsupportedReason = "the field is not scalar";
  }
  else if (!isStructured && !isPointBased)
  {
    unsupportedReason = "cell fields are only supported on 3D structured cell sets";
  }
  else if (!isStructured && !cells.IsType<viskores::cont::CellSetSingleType<>>() &&
           !cells.IsType<viskores::cont::CellSetExplicit<>>())
  {
    unsupportedReason = "the cell set type is not supported";
  }
  else if (!isStructured)
  {
    const bool copied = cells.IsType<viskores::cont::CellSetSingleType<>>()
      ? CopyUnstructuredCells(cells.AsCellSet<viskores::cont::CellSetSingleType<>>(),
                              unstructuredArrays)
      : CopyUnstructuredCells(cells.AsCellSet<viskores::cont::CellSetExplicit<>>(),
                              unstructuredArrays);
    if (!copied)
    {
      unsupportedReason = "the connectivity does not fit in the UInt32 indices ANARI requires";
    }
  }

  if (unsupportedReason)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "ANARIMapperVolume '" << this->GetName()
                                         << "' cannot map its actor: " << unsupportedReason);
    // Drop the volume's reference to the spatial field of an earlier actor.
    if (this->Handles->Volume)
    {
      anari_cpp::unsetParameter(d, this->Handles->Volume, "field");
      anari_cpp::unsetParameter(d, this->Handles->Volume, "value");
      anari_cpp::commitParameters(d, this->Handles->Volume);
    }
    this->RefreshGroup();
    return;
  }

  // Structured regular volume data
  if (isStructured)
  {
    this->Handles->SpatialField =
      anari_cpp::newObject<anari_cpp::SpatialField>(this->GetDevice(), "structuredRegular");

    auto structuredCells = cells.AsCellSet<viskores::cont::CellSetStructured<3>>();
    auto pdims =
      isPointBased ? structuredCells.GetPointDimensions() : structuredCells.GetCellDimensions();

    viskores::cont::ArrayHandle<viskores::Float32> data;
    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, data);

    auto bounds = coords.GetBounds();
    viskores::Vec3f_32 bLower(bounds.X.Min, bounds.Y.Min, bounds.Z.Min);
    viskores::Vec3f_32 bUpper(bounds.X.Max, bounds.Y.Max, bounds.Z.Max);
    viskores::Vec3f_32 size = bUpper - bLower;

    viskores::Vec3ui_32 dims(pdims[0], pdims[1], pdims[2]);
    auto spacing = size / (viskores::Vec3f_32(dims) - 1.f);

    std::memcpy(this->Handles->StructuredParameters.Dims, &dims, sizeof(dims));
    std::memcpy(this->Handles->StructuredParameters.Origin, &bLower, sizeof(bLower));
    std::memcpy(this->Handles->StructuredParameters.Spacing, &spacing, sizeof(spacing));
    this->Handles->StructuredParameters.Data = MakeANARIArray3D(d, data, dims[0], dims[1], dims[2]);

    this->Valid = true;
  }
  // Unstructured volume data
  else
  {
    this->Handles->SpatialField =
      anari_cpp::newObject<anari_cpp::SpatialField>(this->GetDevice(), "unstructured");

    // Vetrex Coordinates
    viskores::cont::ArrayCopyShallowIfPossible(coords.GetData(), unstructuredArrays.VertexPosition);

    // Vetrex Data
    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, unstructuredArrays.VertexData);

    // "indexPrefixed"
    this->Handles->UnstructuredParameters.IndexPrefixed = false;

    auto& parameters = this->Handles->UnstructuredParameters;

    // "vertex.position"
    parameters.VertexPosition = MakeANARIArray1D(d, unstructuredArrays.VertexPosition);

    // "vertex.data"
    parameters.VertexData = MakeANARIArray1D(d, unstructuredArrays.VertexData);

    // "index"
    parameters.Index = MakeANARIArray1D(d, unstructuredArrays.Index);

    // "cell.index": the offsets array without its final entry
    parameters.CellIndex = MakeANARIArray1D(
      d,
      unstructuredArrays.CellIndex,
      ANARI_UINT32,
      static_cast<std::uint64_t>(unstructuredArrays.CellIndex.GetNumberOfValues() - 1));

    // TODO "cell.data" (NOT SUPPORTED YET)
    // parameters.CellData = MakeANARIArray1D(d, unstructuredArrays.CellData);

    // "cell.type"
    parameters.CellType =
      MakeANARIArray1D(d,
                       unstructuredArrays.CellType,
                       ANARI_UINT8,
                       static_cast<std::uint64_t>(unstructuredArrays.CellType.GetNumberOfValues()));

    this->Valid = true;
  }

  this->UpdateSpatialField();
  this->RefreshGroup();
}

void ANARIMapperVolume::UpdateSpatialField()
{
  if (!this->Handles->SpatialField)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "origin");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "spacing");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "data");

  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "vertex.data");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "index");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "indexPrefixed");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "cell.index");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "cell.data");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "cell.type");

  anari_cpp::setParameter(
    d, this->Handles->SpatialField, "name", this->MakeObjectName("spatialField"));

  if (this->Handles->StructuredParameters.Data)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "origin", this->Handles->StructuredParameters.Origin);
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "spacing", this->Handles->StructuredParameters.Spacing);
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "data", this->Handles->StructuredParameters.Data);
  }

  if (this->Handles->UnstructuredParameters.VertexPosition)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "vertex.position",
                            this->Handles->UnstructuredParameters.VertexPosition);
  }
  if (this->Handles->UnstructuredParameters.VertexData)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "vertex.data",
                            this->Handles->UnstructuredParameters.VertexData);
  }
  if (this->Handles->UnstructuredParameters.Index)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "index", this->Handles->UnstructuredParameters.Index);
  }
  if (this->Handles->UnstructuredParameters.CellIndex)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "indexPrefixed",
                            this->Handles->UnstructuredParameters.IndexPrefixed);
  }
  if (this->Handles->UnstructuredParameters.CellIndex)
  {
    anari_cpp::setParameter(d,
                            this->Handles->SpatialField,
                            "cell.index",
                            this->Handles->UnstructuredParameters.CellIndex);
  }
  // if (this->Handles->UnstructuredParameters.CellData)
  // {
  //   anari_cpp::setParameter(
  //     d, this->Handles->SpatialField, "cell.data", this->Handles->UnstructuredParameters.CellData);
  // }
  if (this->Handles->UnstructuredParameters.CellType)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "cell.type", this->Handles->UnstructuredParameters.CellType);
  }

  anari_cpp::commitParameters(d, this->Handles->SpatialField);

  if (this->Handles->Volume)
  {
    anari_cpp::setParameter(d, this->Handles->Volume, "field", this->GetANARISpatialField());
    anari_cpp::setParameter(d, this->Handles->Volume, "value", this->GetANARISpatialField());
    anari_cpp::commitParameters(d, this->Handles->Volume);
  }
}

ANARIMapperVolume::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Volume);
  anari_cpp::release(this->Device, this->SpatialField);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperVolume::ANARIHandles::ReleaseArrays()
{
  auto releaseArray = [this](auto& array)
  {
    anari_cpp::release(this->Device, array);
    array = nullptr;
  };

  releaseArray(this->StructuredParameters.Data);

  releaseArray(this->UnstructuredParameters.VertexPosition);
  releaseArray(this->UnstructuredParameters.VertexData);
  releaseArray(this->UnstructuredParameters.Index);
  releaseArray(this->UnstructuredParameters.CellIndex);
  releaseArray(this->UnstructuredParameters.CellData);
  releaseArray(this->UnstructuredParameters.CellType);
  this->UnstructuredParameters.IndexPrefixed = false;
}

} // namespace anari
} // namespace interop
} // namespace viskores
