//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_FilterTraits_h
#define vtk_m_filter_FilterTraits_h

#include <vtkm/TypeListTag.h>

namespace vtkm
{
namespace filter
{

template <typename Filter>
class FilterTraits
{
public:
  // A filter is able to state what subset of types it supports
  // by default. By default we use ListTagUniversal to represent that the
  // filter accepts all types specified by the users provided policy
  using InputFieldTypeList = vtkm::ListTagUniversal;
};

template <typename DerivedPolicy, typename FilterType>
struct DeduceFilterFieldTypes
{
  using FList = typename vtkm::filter::FilterTraits<FilterType>::InputFieldTypeList;
  using PList = typename DerivedPolicy::FieldTypeList;

  using TypeList = vtkm::ListTagIntersect<FList, PList>;
};
}
}

#endif //vtk_m_filter_FilterTraits_h
