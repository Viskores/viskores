//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellSet_h
#define vtk_m_cont_CellSet_h

#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/LogicalStructure.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/type_traits/is_base_of.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {

class CellSet
{
public:
  VTKM_CONT_EXPORT
  CellSet(const std::string &name, vtkm::IdComponent dimensionality)
    : Name(name), Dimensionality(dimensionality), LogicalStructure()
  {
  }

  virtual ~CellSet()
  {
  }

  virtual std::string GetName() const
  {
    return this->Name;
  }
  virtual vtkm::IdComponent GetDimensionality() const
  {
    return this->Dimensionality;
  }

  virtual vtkm::Id GetNumberOfCells() const = 0;

  virtual vtkm::Id GetNumberOfFaces() const
  {
    return 0;
  }

  virtual vtkm::Id GetNumberOfEdges() const
  {
    return 0;
  }

  // A cell set does not (necessarily) know the number of points. Nor does a
  // DataSet. Shouldn't someone know?
//  virtual vtkm::Id GetNumberOfPoints() const = 0;

  virtual void PrintSummary(std::ostream&) const = 0;

protected:
    std::string Name;
    vtkm::IdComponent Dimensionality;
    vtkm::cont::LogicalStructure LogicalStructure;
};

namespace internal {

/// Checks to see if the given object is a cell set. This check is compatible
/// with the Boost meta-template programming library (MPL). It contains a
/// typedef named \c type that is either boost::mpl::true_ or
/// boost::mpl::false_. Both of these have a typedef named value with the
/// respective boolean value.
///
template<typename T>
struct CellSetCheck
{
  typedef typename boost::is_base_of<vtkm::cont::CellSet, T>::type type;
};

#define VTKM_IS_CELL_SET(T) \
  VTKM_STATIC_ASSERT(::vtkm::cont::internal::CellSetCheck<T>::type::value)

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSet_h
