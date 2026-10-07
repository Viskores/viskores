//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_cont_internal_ArrayTransfer_h
#define viskores_cont_internal_ArrayTransfer_h

#include <viskores/cont/Storage.h>
#include <viskores/cont/Token.h>

namespace viskores
{
namespace cont
{
namespace internal
{

/// \brief Class that manages the transfer of data between control and execution (obsolete).
///
template <typename T, class StorageTag, class DeviceAdapterTag>
class ArrayTransfer
{
  VISKORES_STATIC_ASSERT_MSG(sizeof(T) == static_cast<std::size_t>(-1),
                             "Default implementation of ArrayTransfer no longer available.");
};

}
}
} // namespace viskores::cont::internal

#endif //viskores_cont_internal_ArrayTransfer_h
