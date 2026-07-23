//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_exec_TwoLevelUniformGridExecutonObject_h
#define viskores_exec_TwoLevelUniformGridExecutonObject_h

#include <viskores/cont/ArrayHandle.h>



namespace viskores
{
namespace exec
{
namespace twolevelgrid
{
using DimensionType = viskores::Int16;
using DimVec3 = viskores::Vec<DimensionType, 3>;
using FloatVec3 = viskores::Vec3f;

struct Grid
{
  DimVec3 Dimensions;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};
template <typename Device>
struct TwoLevelUniformGridExecutionObject
{


  template <typename T>
  using ArrayPortalConst = typename viskores::cont::ArrayHandle<T>::ReadPortalType;

  Grid TopLevel;

  ArrayPortalConst<DimVec3> LeafDimensions;
  ArrayPortalConst<viskores::Id> LeafStartIndex;

  ArrayPortalConst<viskores::Id> CellStartIndex;
  ArrayPortalConst<viskores::Id> CellCount;
  ArrayPortalConst<viskores::Id> CellIds;
};
}
}
}
#endif // viskores_cont_TwoLevelUniformGridExecutonObject_h
