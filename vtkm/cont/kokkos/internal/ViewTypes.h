//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_kokkos_internal_ViewTypes_h
#define vtk_m_cont_kokkos_internal_ViewTypes_h

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/Logging.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace kokkos
{
namespace internal
{

template <typename ValueType>
using KokkosViewCont = Kokkos::
  View<ValueType*, Kokkos::LayoutRight, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;

template <typename ValueType>
using KokkosViewExec =
  decltype(Kokkos::create_mirror(Kokkos::DefaultExecutionSpace{}, KokkosViewCont<ValueType>{}));

template <typename ValueType>
using KokkosViewConstCont = typename KokkosViewCont<ValueType>::const_type;

template <typename ValueType>
using KokkosViewConstExec = typename KokkosViewExec<ValueType>::const_type;
}
}
}
} // vtkm::cont::kokkos::internaL

#endif // vtk_m_cont_kokkos_internal_ViewTypes_h
