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
#ifndef vtk_m_cont_CellSetStructured_h
#define vtk_m_cont_CellSetStructured_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>
#include <vtkm/exec/ConnectivityStructured.h>

namespace vtkm {
namespace cont {

template<vtkm::IdComponent DIMENSION>
class CellSetStructured : public CellSet
{
private:
  typedef vtkm::cont::CellSetStructured<DIMENSION> Thisclass;
  typedef vtkm::internal::ConnectivityStructuredInternals<DIMENSION>
      InternalsType;

public:
  static const vtkm::IdComponent Dimension=DIMENSION;

  typedef typename InternalsType::SchedulingRangeType SchedulingRangeType;

  CellSetStructured(const std::string &name = std::string())
    : CellSet(name), Structure()
  {
  }

  CellSetStructured(const Thisclass &src);

  Thisclass &operator=(const Thisclass &src);

  virtual vtkm::Id GetNumberOfCells() const
  {
    return this->Structure.GetNumberOfCells();
  }

  virtual vtkm::Id GetNumberOfPoints() const
  {
    return this->Structure.GetNumberOfPoints();
  }

  virtual vtkm::Id GetNumberOfFaces() const { return -1; }

  virtual vtkm::Id GetNumberOfEdges() const { return -1; }

  void SetPointDimensions(SchedulingRangeType dimensions)
  {
    this->Structure.SetPointDimensions(dimensions);
  }

  SchedulingRangeType GetPointDimensions()
  {
    return this->Structure.GetPointDimensions();
  }

  SchedulingRangeType GetCellDimensions()
  {
    return this->Structure.GetCellDimensions();
  }

  vtkm::IdComponent
  GetNumberOfPointsInCell(vtkm::Id vtkmNotUsed(cellIndex)=0) const
  {
    return this->Structure.GetNumberOfPointsInCell();
  }

  vtkm::IdComponent GetCellShape() const
  {
    return this->Structure.GetCellShape();
  }

  template<typename TopologyElement>
  SchedulingRangeType GetSchedulingRange(TopologyElement) const;

  template<typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);
    typedef vtkm::exec::ConnectivityStructured<FromTopology,ToTopology,Dimension> ExecObjectType;
  };

  template<typename DeviceAdapter, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<DeviceAdapter,FromTopology,ToTopology>::ExecObjectType
  PrepareForInput(DeviceAdapter, FromTopology, ToTopology) const;

  virtual void PrintSummary(std::ostream &out) const;

private:
  InternalsType Structure;
};

#ifndef vtkm_cont_CellSetStructured_cxx
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetStructured<1>;
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetStructured<2>;
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetStructured<3>;
#endif

}
} // namespace vtkm::cont

#include <vtkm/cont/CellSetStructured.hxx>

#endif //vtk_m_cont_CellSetStructured_h
