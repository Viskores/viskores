//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Storage.h>

namespace viskores
{
namespace cont
{
namespace internal
{
namespace detail
{

void StorageNoResizeImpl(viskores::Id currentNumValues,
                         viskores::Id requestedNumValues,
                         std::string storageTagName)
{
  if (currentNumValues == requestedNumValues)
  {
    // Array resized to current size. This is OK.
  }
  else if (requestedNumValues == 0)
  {
    // Array resized to zero. This can happen when releasing resources.
    // Should we try to clear out the buffers, or avoid that for messing up shared buffers?
  }
  else
  {
    throw viskores::cont::ErrorBadAllocation("Cannot resize arrays with storage type of " +
                                             storageTagName);
  }
}

}
}
}
} // namespace viskores::cont::internal::detail
