//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <sstream>
#include <typeindex>

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/VariantArrayHandle.h>

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
void ThrowCastAndCallException(const vtkm::cont::internal::VariantArrayHandleContainerBase& ref,
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
