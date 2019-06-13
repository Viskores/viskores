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

#include <vtkm/TypeListTag.h>

namespace vtkm
{
namespace filter
{

struct DefaultFieldTag
{
};

template <typename Filter, typename FieldTag = DefaultFieldTag>
class FilterTraits
{
public:
  // A filter is able to state what subset of types it supports
  // by default. By default we use ListTagUniversal to represent that the
  // filter accepts all types specified by the users provided policy
  using InputFieldTypeList = vtkm::ListTagUniversal;
};

template <typename DerivedPolicy, typename FilterType, typename FieldTag>
struct DeduceFilterFieldTypes
{
  using FList = typename vtkm::filter::FilterTraits<FilterType, FieldTag>::InputFieldTypeList;
  using PList = typename DerivedPolicy::FieldTypeList;

  using TypeList = vtkm::ListTagIntersect<FList, PList>;
};
}
}

#endif //vtk_m_filter_FilterTraits_h
