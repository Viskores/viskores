//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_IndicesExtrude_h
#define vtk_m_internal_IndicesExtrude_h

#include <vtkm/Math.h>
#include <vtkm/Types.h>

namespace vtkm
{
namespace exec
{

struct IndicesExtrude
{
  IndicesExtrude() = default;

  VTKM_EXEC
  IndicesExtrude(vtkm::Vec3i_32 pointIds1,
                 vtkm::Int32 plane1,
                 vtkm::Vec3i_32 pointIds2,
                 vtkm::Int32 plane2,
                 vtkm::Int32 numberOfPointsPerPlane)
    : PointIds{ pointIds1, pointIds2 }
    , Planes{ plane1, plane2 }
    , NumberOfPointsPerPlane(numberOfPointsPerPlane)
  {
  }

  VTKM_EXEC
  vtkm::Id operator[](vtkm::IdComponent index) const
  {
    VTKM_ASSERT(index >= 0 && index < 6);
    if (index < 3)
    {
      return (static_cast<vtkm::Id>(this->NumberOfPointsPerPlane) * this->Planes[0]) +
        this->PointIds[0][index];
    }
    else
    {
      return (static_cast<vtkm::Id>(this->NumberOfPointsPerPlane) * this->Planes[1]) +
        this->PointIds[1][index - 3];
    }
  }

  VTKM_EXEC
  constexpr vtkm::IdComponent GetNumberOfComponents() const { return 6; }

  template <typename T, vtkm::IdComponent DestSize>
  VTKM_EXEC void CopyInto(vtkm::Vec<T, DestSize>& dest) const
  {
    for (vtkm::IdComponent i = 0; i < vtkm::Min(6, DestSize); ++i)
    {
      dest[i] = (*this)[i];
    }
  }

  vtkm::Vec3i_32 PointIds[2];
  vtkm::Int32 Planes[2];
  vtkm::Int32 NumberOfPointsPerPlane;
};

template <typename ConnectivityPortalType>
struct ReverseIndicesExtrude
{
  ReverseIndicesExtrude() = default;

  VTKM_EXEC
  ReverseIndicesExtrude(const ConnectivityPortalType conn,
                        vtkm::Id offset1,
                        vtkm::IdComponent length1,
                        vtkm::Id offset2,
                        vtkm::IdComponent length2,
                        vtkm::IdComponent plane1,
                        vtkm::IdComponent plane2,
                        vtkm::Int32 numberOfCellsPerPlane)
    : Connectivity(conn)
    , Offset1(offset1)
    , Offset2(offset2)
    , Length1(length1)
    , NumberOfComponents(length1 + length2)
    , CellOffset1(plane1 * numberOfCellsPerPlane)
    , CellOffset2(plane2 * numberOfCellsPerPlane)
  {
  }

  VTKM_EXEC
  vtkm::Id operator[](vtkm::IdComponent index) const
  {
    VTKM_ASSERT(index >= 0 && index < (this->NumberOfComponents));
    if (index < this->Length1)
    {
      return this->Connectivity.Get(this->Offset1 + index) + this->CellOffset1;
    }
    else
    {
      return this->Connectivity.Get(this->Offset2 + index - this->Length1) + this->CellOffset2;
    }
  }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfComponents() const { return this->NumberOfComponents; }

  template <typename T, vtkm::IdComponent DestSize>
  VTKM_EXEC void CopyInto(vtkm::Vec<T, DestSize>& dest) const
  {
    for (vtkm::IdComponent i = 0; i < vtkm::Min(this->NumberOfComponents, DestSize); ++i)
    {
      dest[i] = (*this)[i];
    }
  }

  ConnectivityPortalType Connectivity;
  vtkm::Id Offset1, Offset2;
  vtkm::IdComponent Length1;
  vtkm::IdComponent NumberOfComponents;
  vtkm::Id CellOffset1, CellOffset2;
};
}
}

#endif //vtkm_m_internal_IndicesExtrude_h
