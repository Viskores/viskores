//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================


#ifndef vtk_m_RegularConnectivity_h
#define vtk_m_RegularConnectivity_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {

template<vtkm::IdComponent Dimension>
struct SchedulingDimension
{
  typedef vtkm::Vec<vtkm::Id, Dimension> ValueType;
};

template<>
struct SchedulingDimension<1>
{
  typedef vtkm::Id ValueType;
};

template<typename FromTopology,
         typename ToTopology,
         vtkm::IdComponent Dimension>
class RegularConnectivity
{
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
  VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

  typedef vtkm::internal::ConnectivityStructuredInternals<Dimension>
      InternalsType;

public:
  typedef typename SchedulingDimension<Dimension>::ValueType SchedulingDimension;

  RegularConnectivity():
    Internals()
  {

  }

  RegularConnectivity(const InternalsType &src):
    Internals(src)
  {
  }

  RegularConnectivity(const RegularConnectivity &src):
    Internals(src.Internals)
  {
  }

  VTKM_EXEC_CONT_EXPORT
  SchedulingDimension GetSchedulingDimensions() const {
    return Internals.GetSchedulingDimensions();
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfIndices(vtkm::Id index) const {
    typedef vtkm::internal::ConnectivityStructuredIndexHelper<
        FromTopology,ToTopology,Dimension> Helper;
    return Helper::GetNumberOfIndices(this->Internals, index);
  }
  // This needs some thought. What does cell shape mean when the to topology
  // is not a cell?
  VTKM_EXEC_CONT_EXPORT
  vtkm::CellType GetCellShapeType(vtkm::Id=0) const {
    return Internals.GetCellShapeType();
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_EXEC_CONT_EXPORT
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    typedef vtkm::internal::ConnectivityStructuredIndexHelper<
        FromTopology,ToTopology,Dimension> Helper;
    Helper::GetIndices(this->Internals,index,ids);
  }

  template <typename DeviceAdapterTag>
  struct ExecutionTypes
  {
    typedef vtkm::RegularConnectivity<FromTopology,ToTopology,Dimension> ExecObjectType;
  };

  template<typename DeviceAdapterTag>
  typename ExecutionTypes<DeviceAdapterTag>::ExecObjectType
  PrepareForInput(DeviceAdapterTag) const
  {
    return typename ExecutionTypes<DeviceAdapterTag>::ExecObjectType(*this);
  }

private:
  InternalsType Internals;
};

} // namespace vtkm

#endif //vtk_m_RegularConnectivity_h
