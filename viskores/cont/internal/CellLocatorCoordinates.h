//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_internal_CellLocatorCoordinates_h
#define viskores_cont_internal_CellLocatorCoordinates_h

#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/CoordinateSystem.h>

#include <viskores/exec/Variant.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template <typename CoordinateType>
struct StorageToCellLocatorCoordinateArray
{
  using ValueType = viskores::Vec<CoordinateType, 3>;

  template <typename Storage>
  using IsInvalid = viskores::cont::internal::IsInvalidArrayHandle<ValueType, Storage>;

  template <typename Storage>
  using Transform = viskores::cont::ArrayHandle<ValueType, Storage>;
};

template <typename CoordinateType>
using CellLocatorCoordinateArrayList = viskores::ListTransform<
  viskores::ListRemoveIf<VISKORES_DEFAULT_STORAGE_LIST,
                         StorageToCellLocatorCoordinateArray<CoordinateType>::template IsInvalid>,
  StorageToCellLocatorCoordinateArray<CoordinateType>::template Transform>;

template <typename CoordinateType>
using CellLocatorCoordinateArrayMultiplexer =
  viskores::cont::ArrayHandleMultiplexerFromList<CellLocatorCoordinateArrayList<CoordinateType>>;

using CellLocatorCoordinatePortalVariant = viskores::exec::Variant<
  typename CellLocatorCoordinateArrayMultiplexer<viskores::Float32>::ReadPortalType,
  typename CellLocatorCoordinateArrayMultiplexer<viskores::Float64>::ReadPortalType>;

/// Prepare coordinates for execution without changing their scalar type.
///
/// The returned variant contains a storage multiplexer portal for either
/// `Float32` or `Float64` coordinates. The multiplexer retains the original
/// array storage rather than materializing a converted coordinate array.
VISKORES_CONT_EXPORT CellLocatorCoordinatePortalVariant
PrepareCellLocatorCoordinates(const viskores::cont::CoordinateSystem& coordinates,
                              viskores::cont::DeviceAdapterId device,
                              viskores::cont::Token& token);

/// Prepare an existing coordinate multiplexer while recovering its native scalar type.
///
/// This overload preserves compatibility with cell locator execution objects
/// constructed from `CoordinateSystem::MultiplexerArrayType`.
VISKORES_CONT_EXPORT CellLocatorCoordinatePortalVariant PrepareCellLocatorCoordinates(
  const viskores::cont::CoordinateSystem::MultiplexerArrayType& coordinates,
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token);

} // namespace internal
} // namespace cont
} // namespace viskores

#endif // viskores_cont_internal_CellLocatorCoordinates_h
