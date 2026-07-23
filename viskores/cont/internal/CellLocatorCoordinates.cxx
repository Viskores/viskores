//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/internal/CellLocatorCoordinates.h>

namespace viskores
{
namespace cont
{
namespace internal
{

namespace
{

struct PrepareCoordinatesForExecution
{
  template <typename CoordinatesArrayType>
  VISKORES_CONT void operator()(
    const CoordinatesArrayType& coordinates,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token,
    viskores::cont::internal::CellLocatorCoordinatePortalVariant& executionObject) const
  {
    using CoordinateType =
      typename viskores::VecTraits<typename CoordinatesArrayType::ValueType>::ComponentType;
    using MultiplexerType =
      viskores::cont::internal::CellLocatorCoordinateArrayMultiplexer<CoordinateType>;

    executionObject = MultiplexerType(coordinates).PrepareForInput(device, token);
  }

  template <typename TargetType, typename SourceType, typename SourceStorage>
  VISKORES_CONT void operator()(
    const viskores::cont::ArrayHandle<TargetType,
                                      viskores::cont::StorageTagCast<SourceType, SourceStorage>>&
      coordinates,
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token,
    viskores::cont::internal::CellLocatorCoordinatePortalVariant& executionObject) const
  {
    using SourceArrayType = viskores::cont::ArrayHandle<SourceType, SourceStorage>;
    using CastArrayType = viskores::cont::ArrayHandleCast<TargetType, SourceArrayType>;

    const CastArrayType castCoordinates = coordinates;
    (*this)(castCoordinates.GetSourceArray(), device, token, executionObject);
  }
};

} // anonymous namespace

CellLocatorCoordinatePortalVariant PrepareCellLocatorCoordinates(
  const viskores::cont::CoordinateSystem& coordinates,
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  CellLocatorCoordinatePortalVariant executionObject;
  coordinates.GetData().CastAndCall(
    PrepareCoordinatesForExecution{}, device, token, executionObject);
  return executionObject;
}

CellLocatorCoordinatePortalVariant PrepareCellLocatorCoordinates(
  const viskores::cont::CoordinateSystem::MultiplexerArrayType& coordinates,
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  CellLocatorCoordinatePortalVariant executionObject;
  coordinates.GetArrayHandleVariant().CastAndCall(
    PrepareCoordinatesForExecution{}, device, token, executionObject);
  return executionObject;
}

} // namespace internal
} // namespace cont
} // namespace viskores
