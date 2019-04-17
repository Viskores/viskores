//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <sstream>
#include <typeindex>

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/internal/VariantArrayHandleContainer.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

VariantArrayHandleContainerBase::VariantArrayHandleContainerBase()
  : TypeIndex(typeid(nullptr))
{
}

VariantArrayHandleContainerBase::VariantArrayHandleContainerBase(const std::type_info& typeinfo)
  : TypeIndex(typeinfo)
{
}

VariantArrayHandleContainerBase::~VariantArrayHandleContainerBase()
{
}
}

namespace detail
{
VTKM_CONT_EXPORT void ThrowCastAndCallException(
  const vtkm::cont::internal::VariantArrayHandleContainerBase& ref,
  const std::type_info& type)
{
  std::ostringstream out;
  out << "Could not find appropriate cast for array in CastAndCall1.\n"
         "Array: ";
  ref.PrintSummary(out);
  out << "TypeList: " << type.name() << "\n";
  throw vtkm::cont::ErrorBadValue(out.str());
}
}
}
} // namespace vtkm::cont::detail
