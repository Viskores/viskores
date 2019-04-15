//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_StaticIndex_h
#define vtk_m_internal_StaticIndex_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace internal
{

/// \brief A convenience tag to represent static indices.
///
/// Some classes like \c FunctionInterface have a list of items that have
/// numeric indices that must be resolved at compile time. Typically these are
/// referenced with an integer template argument. However, such template
/// arguments have to be explicitly defined in the template. They cannot be
/// resolved through function or method arguments. In such cases, it is
/// convenient to use this tag to encapsulate the index.
///
template <vtkm::IdComponent Index>
struct IndexTag
{
  static constexpr vtkm::IdComponent INDEX = Index;
};
}
} // namespace vtkm::internal

#endif //vtk_m_internal_StaticIndex_h
