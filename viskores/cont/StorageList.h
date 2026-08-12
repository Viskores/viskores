//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_StorageList_h
#define viskores_cont_StorageList_h

#include <viskores/List.h>

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleSOAStride.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

namespace viskores
{
namespace cont
{

using StorageListBasic = viskores::List<viskores::cont::StorageTagBasic>;

using StorageListCommon =
  viskores::List<viskores::cont::StorageTagBasic,
                 viskores::cont::StorageTagUniformPoints,
                 viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                                            viskores::cont::StorageTagBasic,
                                                            viskores::cont::StorageTagBasic>,
                 viskores::cont::StorageTagSOAStride>;

}
} // namespace viskores::cont

#endif //viskores_cont_StorageList_h
