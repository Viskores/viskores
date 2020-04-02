//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FilterTraits_h
#define vtk_m_filter_FilterTraits_h

#include <vtkm/List.h>

namespace vtkm
{
namespace filter
{

template <typename Derived>
class Filter;


template <typename Filter>
struct FilterTraits
{
  using InputFieldTypeList = typename Filter::SupportedTypes;
  using AdditionalFieldStorage = typename Filter::AdditionalFieldStorage;
};

template <typename DerivedPolicy, typename ListOfTypes>
struct DeduceFilterFieldTypes
{
  using PList = typename DerivedPolicy::FieldTypeList;
  using TypeList = vtkm::ListIntersect<ListOfTypes, PList>;
};
}
}

#endif //vtk_m_filter_FilterTraits_h
