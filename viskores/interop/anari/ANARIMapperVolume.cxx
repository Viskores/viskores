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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>

#include <cstring>

namespace viskores
{
namespace interop
{
namespace anari
{

namespace
{

bool StringListContains(const char* const* values, const char* expected)
{
  if (!values)
  {
    return false;
  }
  for (const char* const* value = values; *value != nullptr; ++value)
  {
    if (std::strcmp(*value, expected) == 0)
    {
      return true;
    }
  }
  return false;
}

bool ActorIsEmpty(const ANARIActor& actor)
{
  return actor.GetCellSet().GetNumberOfCells() == 0 &&
    actor.GetCoordinateSystem().GetNumberOfPoints() == 0 &&
    actor.GetField().GetNumberOfValues() == 0;
}

void RequireANARIObjectSupport(anari_cpp::Device device,
                               ANARIDataType objectType,
                               const char* subtype,
                               const char* extension)
{
  const char** extensions = nullptr;
  const auto extensionsAvailable = anariGetProperty(
    device, device, "extension", ANARI_STRING_LIST, &extensions, sizeof(extensions), ANARI_WAIT);
  if (extensionsAvailable == 0 || !StringListContains(extensions, extension))
  {
    throw viskores::cont::ErrorBadValue(std::string("ANARI device does not support required ") +
                                        extension + " extension.");
  }

  const char** subtypes = anariGetObjectSubtypes(device, objectType);
  if (!StringListContains(subtypes, subtype))
  {
    throw viskores::cont::ErrorBadValue(std::string("ANARI device does not support required '") +
                                        subtype + "' subtype.");
  }
}

} // anonymous namespace

ANARIMapperVolume::ANARIMapperVolume(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_unique<ANARIMapperVolume::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperVolume::ANARIMapperVolume(ANARIMapperVolume&&) = default;

ANARIMapperVolume::~ANARIMapperVolume()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperVolume::SetActor(const ANARIActor& actor)
{
  const ANARIActor previousActor = this->GetActor();
  const bool previousCurrent = this->Current;
  this->ANARIMapper::SetActor(actor);
  try
  {
    this->ConstructArrays(true);
  }
  catch (...)
  {
    this->ANARIMapper::SetActor(previousActor);
    this->Current = previousCurrent;
    throw;
  }
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
  if (this->Handles->Field)
  {
    return this->Handles->Field->SpatialField;
  }

  this->ConstructArrays();
  return this->Handles->Field ? this->Handles->Field->SpatialField : nullptr;
}

anari_cpp::Volume ANARIMapperVolume::GetANARIVolume()
{
  if (this->Handles->Volume)
    return this->Handles->Volume;

  if (ActorIsEmpty(this->GetActor()))
  {
    this->Current = true;
    this->Valid = false;
    return nullptr;
  }

  auto d = this->GetDevice();

  RequireANARIObjectSupport(
    d, ANARI_VOLUME, "transferFunction1D", "ANARI_KHR_VOLUME_TRANSFER_FUNCTION1D");
  auto volume = anari_cpp::newObject<anari_cpp::Volume>(d, "transferFunction1D");
  this->Handles->Volume = volume;

  try
  {
    auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
    auto* colors = anari_cpp::map<viskores::Vec3f_32>(d, colorArray);
    colors[0] = viskores::Vec3f_32(1.f, 0.f, 0.f);
    colors[1] = viskores::Vec3f_32(0.f, 1.f, 0.f);
    colors[2] = viskores::Vec3f_32(0.f, 0.f, 1.f);
    anari_cpp::unmap(d, colorArray);

    auto opacityArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
    auto* opacities = anari_cpp::map<viskores::Float32>(d, opacityArray);
    opacities[0] = 0.f;
    opacities[1] = 1.f;
    anari_cpp::unmap(d, opacityArray);

    anari_cpp::setAndReleaseParameter(d, volume, "color", colorArray);
    anari_cpp::setAndReleaseParameter(d, volume, "opacity", opacityArray);
    auto spatialField = this->GetANARISpatialField();
    // Keep old name as a parameter for bug-backwards compatibility: 'field' isn't the right name
    anari_cpp::setParameter(d, volume, "field", spatialField);
    anari_cpp::setParameter(d, volume, "value", spatialField);
    anari_cpp::setParameter(d, volume, "name", this->MakeObjectName("volume"));
    const viskores::Vec2f_32 valueRange(0.f, 10.f);
    anari_cpp::setParameter(d, volume, "valueRange", ANARI_FLOAT32_BOX1, &valueRange);
    anari_cpp::commitParameters(d, volume);
  }
  catch (...)
  {
    anari_cpp::release(d, volume);
    this->Handles->Volume = nullptr;
    throw;
  }

  return this->Handles->Volume;
}

// For the moment, we use ospray conventions
//    uint8_t VKL_TETRAHEDRON = 10;
//    uint8_t VKL_HEXAHEDRON = 12;
//    uint8_t VKL_WEDGE = 13;
//    uint8_t VKL_PYRAMID = 14;
//
struct ToAnariCellType
{
  VISKORES_EXEC_CONT viskores::UInt8 operator()(viskores::UInt8 shape) const
  {
    if (shape == viskores::CELL_SHAPE_TETRA)
    {
      return 10;
    }
    else if (shape == viskores::CELL_SHAPE_HEXAHEDRON)
    {
      return 14;
    }
    else if (shape == viskores::CELL_SHAPE_WEDGE)
    {
      return 13;
    }
    else if (shape == viskores::CELL_SHAPE_PYRAMID)
    {
      return 12;
    }
    return uint8_t(-1);
  }
};

void ANARIMapperVolume::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  auto d = this->GetDevice();

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();
  const auto& cells = actor.GetCellSet();
  const auto& fieldArray = actor.GetField().GetData();

  if (ActorIsEmpty(actor))
  {
    if (this->Handles->Volume)
    {
      anari_cpp::unsetParameter(d, this->Handles->Volume, "field");
      anari_cpp::unsetParameter(d, this->Handles->Volume, "value");
      anari_cpp::commitParameters(d, this->Handles->Volume);
    }
    this->Handles->Field.reset();
    this->Current = true;
    this->Valid = false;
    this->RefreshGroup();
    return;
  }

  const bool isPointBased =
    actor.GetField().GetAssociation() == viskores::cont::Field::Association::Points;
  const bool isStructured = cells.CanConvert<viskores::cont::CellSetStructured<3>>();
  const bool isScalar = fieldArray.GetNumberOfComponentsFlat() == 1;

  if (fieldArray.GetNumberOfValues() == 0)
  {
    throw viskores::cont::ErrorBadValue("ANARI volumes require a non-empty field.");
  }
  if (!isPointBased || !isScalar)
  {
    throw viskores::cont::ErrorBadValue("ANARI volumes require a point-associated scalar field.");
  }
  if (isStructured &&
      !coords.GetData().IsType<viskores::cont::ArrayHandleUniformPointCoordinates>())
  {
    throw viskores::cont::ErrorBadValue(
      "ANARI structuredRegular volumes require uniform point coordinates.");
  }

  auto nextState = std::make_unique<ANARISpatialFieldState>();
  nextState->Device = d;

  // Structured regular volume data
  if (isStructured)
  {
    auto structuredCells = cells.AsCellSet<viskores::cont::CellSetStructured<3>>();
    auto pdims = structuredCells.GetPointDimensions();
    if (pdims[0] < 2 || pdims[1] < 2 || pdims[2] < 2)
    {
      throw viskores::cont::ErrorBadValue(
        "ANARI structuredRegular volumes require at least two samples per dimension.");
    }
    if (fieldArray.GetNumberOfValues() != pdims[0] * pdims[1] * pdims[2])
    {
      throw viskores::cont::ErrorBadValue(
        "ANARI structuredRegular volumes require one field value per point.");
    }

    auto uniformCoordinates =
      coords.GetData().AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
    if (uniformCoordinates.GetDimensions() != pdims)
    {
      throw viskores::cont::ErrorBadValue(
        "ANARI structuredRegular coordinate and cell-set dimensions must match.");
    }

    RequireANARIObjectSupport(
      d, ANARI_SPATIAL_FIELD, "structuredRegular", "ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR");
    nextState->SpatialField = anari_cpp::newObject<anari_cpp::SpatialField>(d, "structuredRegular");

    auto& arrays = nextState->StructuredArrays;

    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, arrays.Data);
    auto* ptr = static_cast<const viskores::Float32*>(
      arrays.Data.GetBuffers()[0].ReadPointerHost(*arrays.Token));

    viskores::Vec3ui_32 dims(pdims[0], pdims[1], pdims[2]);
    viskores::Vec3f_32 origin(uniformCoordinates.GetOrigin());
    viskores::Vec3f_32 spacing(uniformCoordinates.GetSpacing());

    std::memcpy(nextState->StructuredParameters.Dims, &dims, sizeof(dims));
    std::memcpy(nextState->StructuredParameters.Origin, &origin, sizeof(origin));
    std::memcpy(nextState->StructuredParameters.Spacing, &spacing, sizeof(spacing));
    nextState->StructuredParameters.Data =
      anari_cpp::newArray3D(d, ptr, NoopANARIDeleter, nullptr, dims[0], dims[1], dims[2]);
  }
  // Unstructured volume data
  else
  {
    RequireANARIObjectSupport(
      d, ANARI_SPATIAL_FIELD, "unstructured", "ANARI_KHR_SPATIAL_FIELD_UNSTRUCTURED");
    nextState->SpatialField = anari_cpp::newObject<anari_cpp::SpatialField>(d, "unstructured");

    auto& arrays = nextState->UnstructuredArrays;

    // Cell Data
    if (cells.IsType<viskores::cont::CellSetSingleType<>>())
    {
      // 1. Cell Type
      viskores::cont::CellSetSingleType<> sgl =
        cells.AsCellSet<viskores::cont::CellSetSingleType<>>();
      auto shapes =
        sgl.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(
        viskores::cont::make_ArrayHandleTransform(shapes, ToAnariCellType{}), arrays.CellType);

      // 2. Cell Connectivity
      auto conn = sgl.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                           viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(conn, arrays.Index);

      // 3. Cell Index
      auto offsets = sgl.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                         viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(offsets, arrays.CellIndex);
    }

    else if (cells.IsType<viskores::cont::CellSetExplicit<>>())
    {
      // 1. Cell Type
      viskores::cont::CellSetExplicit<> exp = cells.AsCellSet<viskores::cont::CellSetExplicit<>>();
      auto shapes =
        exp.GetShapesArray(viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(
        viskores::cont::make_ArrayHandleTransform(shapes, ToAnariCellType{}), arrays.CellType);

      // 2. Cell Connectivity
      auto conn = exp.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                           viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(conn, arrays.Index);

      // 3. Cell Index
      auto offsets = exp.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                         viskores::TopologyElementTagPoint());
      viskores::cont::ArrayCopyDevice(offsets, arrays.CellIndex);
    }

    // Vetrex Coordinates
    viskores::cont::ArrayCopyShallowIfPossible(coords.GetData(), arrays.VertexPosition);

    // Vetrex Data
    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, arrays.VertexData);

    // "indexPrefixed"
    nextState->UnstructuredParameters.IndexPrefixed = false;

    // "vertex.position"
    {
      auto* ptr = static_cast<const viskores::Vec3f_32*>(
        arrays.VertexPosition.GetBuffers()[0].ReadPointerHost(*arrays.Token));
      nextState->UnstructuredParameters.VertexPosition = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.VertexPosition.GetNumberOfValues());
    }

    // "vertex.data"
    {
      auto* ptr = static_cast<const viskores::Float32*>(
        arrays.VertexData.GetBuffers()[0].ReadPointerHost(*arrays.Token));
      nextState->UnstructuredParameters.VertexData = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.VertexData.GetNumberOfValues());
    }

    // "index"
    {
      auto* ptr = static_cast<const viskores::UInt64*>(
        arrays.Index.GetBuffers()[0].ReadPointerHost(*arrays.Token));
      nextState->UnstructuredParameters.Index =
        anari_cpp::newArray1D(d, ptr, NoopANARIDeleter, nullptr, arrays.Index.GetNumberOfValues());
    }

    // "cell.index"
    {
      auto* ptr = static_cast<const viskores::UInt64*>(
        arrays.CellIndex.GetBuffers()[0].ReadPointerHost(*arrays.Token));
      nextState->UnstructuredParameters.CellIndex = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.CellIndex.GetNumberOfValues() - 1);
    }

    // TODO "cell.data" (NOT SUPPORED YET)
    // {
    //   auto* ptr = static_cast<const viskores::Float32*>(
    //     arrays.CellData.GetBuffers()[0].ReadPointerHost(*arrays.Token));
    //   nextState->UnstructuredParameters.CellData =
    //     anari_cpp::newArray1D(d, ptr, NoopANARIDeleter, nullptr, arrays.CellData.GetNumberOfValues());
    // }

    // "cell.type"
    {
      auto* ptr = static_cast<const viskores::UInt8*>(
        arrays.CellType.GetBuffers()[0].ReadPointerHost(*arrays.Token));
      nextState->UnstructuredParameters.CellType = anari_cpp::newArray1D(
        d, ptr, NoopANARIDeleter, nullptr, arrays.CellType.GetNumberOfValues());
    }
  }

  this->UpdateSpatialField(*nextState);
  this->Handles->Field = std::move(nextState);
  this->Current = true;
  this->Valid = true;
  this->RefreshGroup();
}

void ANARIMapperVolume::UpdateSpatialField(ANARISpatialFieldState& state)
{
  if (!state.SpatialField)
  {
    return;
  }

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, state.SpatialField, "origin");
  anari_cpp::unsetParameter(d, state.SpatialField, "spacing");
  anari_cpp::unsetParameter(d, state.SpatialField, "data");

  anari_cpp::unsetParameter(d, state.SpatialField, "vertex.position");
  anari_cpp::unsetParameter(d, state.SpatialField, "vertex.data");
  anari_cpp::unsetParameter(d, state.SpatialField, "index");
  anari_cpp::unsetParameter(d, state.SpatialField, "indexPrefixed");
  anari_cpp::unsetParameter(d, state.SpatialField, "cell.index");
  anari_cpp::unsetParameter(d, state.SpatialField, "cell.data");
  anari_cpp::unsetParameter(d, state.SpatialField, "cell.type");

  anari_cpp::setParameter(d, state.SpatialField, "name", this->MakeObjectName("spatialField"));

  if (state.StructuredParameters.Data)
  {
    anari_cpp::setParameter(d, state.SpatialField, "origin", state.StructuredParameters.Origin);
    anari_cpp::setParameter(d, state.SpatialField, "spacing", state.StructuredParameters.Spacing);
    anari_cpp::setParameter(d, state.SpatialField, "data", state.StructuredParameters.Data);
  }

  if (state.UnstructuredParameters.VertexPosition)
  {
    anari_cpp::setParameter(
      d, state.SpatialField, "vertex.position", state.UnstructuredParameters.VertexPosition);
  }
  if (state.UnstructuredParameters.VertexData)
  {
    anari_cpp::setParameter(
      d, state.SpatialField, "vertex.data", state.UnstructuredParameters.VertexData);
  }
  if (state.UnstructuredParameters.Index)
  {
    anari_cpp::setParameter(d, state.SpatialField, "index", state.UnstructuredParameters.Index);
  }
  if (state.UnstructuredParameters.CellIndex)
  {
    anari_cpp::setParameter(
      d, state.SpatialField, "indexPrefixed", state.UnstructuredParameters.IndexPrefixed);
  }
  if (state.UnstructuredParameters.CellIndex)
  {
    anari_cpp::setParameter(
      d, state.SpatialField, "cell.index", state.UnstructuredParameters.CellIndex);
  }
  // if (state.UnstructuredParameters.CellData)
  // {
  //   anari_cpp::setParameter(
  //     d, state.SpatialField, "cell.data", state.UnstructuredParameters.CellData);
  // }
  if (state.UnstructuredParameters.CellType)
  {
    anari_cpp::setParameter(
      d, state.SpatialField, "cell.type", state.UnstructuredParameters.CellType);
  }

  anari_cpp::commitParameters(d, state.SpatialField);

  if (this->Handles->Volume)
  {
    anari_cpp::setParameter(d, this->Handles->Volume, "field", state.SpatialField);
    anari_cpp::setParameter(d, this->Handles->Volume, "value", state.SpatialField);
    anari_cpp::commitParameters(d, this->Handles->Volume);
  }
}

ANARIMapperVolume::ANARIHandles::~ANARIHandles()
{
  anari_cpp::release(this->Device, this->Volume);
  this->Volume = nullptr;
  this->Field.reset();
  anari_cpp::release(this->Device, this->Device);
}

ANARIMapperVolume::ANARISpatialFieldState::~ANARISpatialFieldState()
{
  anari_cpp::release(this->Device, this->SpatialField);
  this->SpatialField = nullptr;
  this->ReleaseArrays();
}

void ANARIMapperVolume::ANARISpatialFieldState::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->StructuredParameters.Data);
  this->StructuredParameters.Data = nullptr;

  anari_cpp::release(this->Device, this->UnstructuredParameters.VertexPosition);
  this->UnstructuredParameters.VertexPosition = nullptr;
  anari_cpp::release(this->Device, this->UnstructuredParameters.VertexData);
  this->UnstructuredParameters.VertexData = nullptr;
  anari_cpp::release(this->Device, this->UnstructuredParameters.Index);
  this->UnstructuredParameters.Index = nullptr;
  anari_cpp::release(this->Device, this->UnstructuredParameters.CellIndex);
  this->UnstructuredParameters.CellIndex = nullptr;
  anari_cpp::release(this->Device, this->UnstructuredParameters.CellData);
  this->UnstructuredParameters.CellData = nullptr;
  anari_cpp::release(this->Device, this->UnstructuredParameters.CellType);
  this->UnstructuredParameters.CellType = nullptr;
}

} // namespace anari
} // namespace interop
} // namespace viskores
