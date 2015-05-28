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
#ifndef vtk_m_Extent_h
#define vtk_m_Extent_h

#include <vtkm/Types.h>

namespace vtkm {

/// Extent stores the values for the index ranges for a structured grid array.
/// It does this through the minimum indices and the maximum indices.
///
template<vtkm::IdComponent Dimensions>
struct Extent
{
  vtkm::Vec<vtkm::Id,Dimensions> Min;
  vtkm::Vec<vtkm::Id,Dimensions> Max;

  VTKM_EXEC_CONT_EXPORT
  Extent() : Min(0), Max(0) {  }

  VTKM_EXEC_CONT_EXPORT
  Extent(const vtkm::Vec<vtkm::Id,Dimensions> &min,
         const vtkm::Vec<vtkm::Id,Dimensions> &max)
    : Min(min), Max(max) {  }

  VTKM_EXEC_CONT_EXPORT
  Extent(const Extent<Dimensions> &other)
    : Min(other.Min), Max(other.Max) {  }

  VTKM_EXEC_CONT_EXPORT
  Extent<Dimensions> &operator=(const Extent<Dimensions> &rhs)
  {
    this->Min = rhs.Min;
    this->Max = rhs.Max;
    return *this;
  }
};

/// This is the Extent to use for structured grids with 3 topological
/// dimensions.
///
typedef vtkm::Extent<3> Extent3;

/// This is the Extent to use for structured grids with 2 topological
/// dimensions.
///
typedef vtkm::Extent<2> Extent2;

/// Given an extent, returns the array dimensions for the points.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<vtkm::Id,Dimensions>
ExtentPointDimensions(const vtkm::Extent<Dimensions> &extent)
{
  return extent.Max - extent.Min + vtkm::Vec<vtkm::Id,Dimensions>(1);
}

VTKM_EXEC_CONT_EXPORT
vtkm::Id3 ExtentPointDimensions(const vtkm::Extent3 &extent)
{
  // Efficient implementation that uses no temporary Id3 to create dimensions.
  return vtkm::Id3(extent.Max[0] - extent.Min[0] + 1,
                   extent.Max[1] - extent.Min[1] + 1,
                   extent.Max[2] - extent.Min[2] + 1);
}

VTKM_EXEC_CONT_EXPORT
vtkm::Id2 ExtentPointDimensions(const vtkm::Extent2 &extent)
{
  // Efficient implementation that uses no temporary Id2 to create dimensions.
  return vtkm::Id2(extent.Max[0] - extent.Min[0] + 1,
                   extent.Max[1] - extent.Min[1] + 1);
}

/// Given an extent, returns the array dimensions for the cells.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<vtkm::Id,Dimensions>
ExtentCellDimensions(const vtkm::Extent<Dimensions> &extent)
{
  return extent.Max - extent.Min;
}

/// Given an extent, returns the number of points in the structured mesh.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentNumberOfPoints(const vtkm::Extent<Dimensions> &extent)
{
  return internal::VecProduct<Dimensions>()(
        vtkm::ExtentPointDimensions(extent));
}

template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentNumberOfPoints(const vtkm::Extent3 &extent)
{
  // Efficient implementation that uses no temporary Id3.
  return (
        (extent.Max[0] - extent.Min[0] + 1)
      * (extent.Max[1] - extent.Min[1] + 1)
      * (extent.Max[2] - extent.Min[2] + 1));
}

template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentNumberOfPoints(const vtkm::Extent2 &extent)
{
  // Efficient implementation that uses no temporary Id2.
  return (
        (extent.Max[0] - extent.Min[0] + 1)
      * (extent.Max[1] - extent.Min[1] + 1));
}

/// Given an extent, returns the number of cells in the structured mesh.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentNumberOfCells(const vtkm::Extent<Dimensions> &extent)
{
  return internal::VecProduct<Dimensions>()(
        vtkm::ExtentCellDimensions(extent));
}

template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentNumberOfCells(const vtkm::Extent3 &extent)
{
  // Efficient implementation that uses no temporary Id3.
  return (
        (extent.Max[0] - extent.Min[0])
      * (extent.Max[1] - extent.Min[1])
      * (extent.Max[2] - extent.Min[2]));
}

template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentNumberOfCells(const vtkm::Extent2 &extent)
{
  // Efficient implementation that uses no temporary Id2.
  return (
        (extent.Max[0] - extent.Min[0])
      * (extent.Max[1] - extent.Min[1]));
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on.  This method converts a flat index to the
/// topological coordinates (e.g. r,s,t for 3d topologies).
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<vtkm::Id,Dimensions>
ExtentPointFlatIndexToTopologyIndex(vtkm::Id index,
                                    const vtkm::Extent<Dimensions> &extent)
{
  const vtkm::Vec<vtkm::Id,Dimensions> dims =
      vtkm::ExtentPointDimensions(extent);
  vtkm::Vec<vtkm::Id,Dimensions> ijkIndex;
  vtkm::Id indexOnDim = index;
  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions-1; dimIndex++)
  {
    ijkIndex[dimIndex] = indexOnDim % dims[dimIndex] + extent.Min[dimIndex];
    indexOnDim /= dims[dimIndex];
  }
  // Special case for last dimension to remove some unneeded operations
  ijkIndex[Dimensions-1] = indexOnDim + extent.Min[Dimensions-1];
  return ijkIndex;
}

VTKM_EXEC_CONT_EXPORT
vtkm::Id3 ExtentPointFlatIndexToTopologyIndex(vtkm::Id index,
                                              const vtkm::Extent3 &extent)
{
  // Efficient implementation that tries to reduce the number of temporary
  // variables.
  const vtkm::Id3 dims = vtkm::ExtentPointDimensions(extent);
  return vtkm::Id3((index % dims[0]) + extent.Min[0],
                   ((index / dims[0]) % dims[1]) + extent.Min[1],
                   ((index / (dims[0]*dims[1]))) + extent.Min[2]);
}

VTKM_EXEC_CONT_EXPORT
vtkm::Id2 ExtentPointFlatIndexToTopologyIndex(vtkm::Id index,
                                              const vtkm::Extent2 &extent)
{
  // Efficient implementation that tries to reduce the number of temporary
  // variables.
  const vtkm::Id2 dims = vtkm::ExtentPointDimensions(extent);
  return vtkm::Id2((index % dims[0]) + extent.Min[0],
                   (index / dims[0]) + extent.Min[1]);
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on.  This method converts a flat index to the
/// topological coordinates (e.g. r,s,t for 3d topologies).
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<vtkm::Id,Dimensions>
ExtentCellFlatIndexToTopologyIndex(vtkm::Id index,
                                   const vtkm::Extent<Dimensions> &extent)
{
  const vtkm::Vec<vtkm::Id,Dimensions> dims =
      vtkm::ExtentCellDimensions(extent);
  vtkm::Vec<vtkm::Id,Dimensions> ijkIndex;
  vtkm::Id indexOnDim = index;
  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions-1; dimIndex++)
  {
    ijkIndex[dimIndex] = indexOnDim % dims[dimIndex] + extent.Min[dimIndex];
    indexOnDim /= dims[dimIndex];
  }
  // Special case for last dimension to remove some unneeded operations
  ijkIndex[Dimensions-1] = indexOnDim + extent.Min[Dimensions-1];
  return ijkIndex;
}

VTKM_EXEC_CONT_EXPORT
vtkm::Id3 ExtentCellFlatIndexToTopologyIndex(vtkm::Id index,
                                             const vtkm::Extent3 &extent)
{
  // Efficient implementation that tries to reduce the number of temporary
  // variables.
  const vtkm::Id3 dims = vtkm::ExtentCellDimensions(extent);
  return vtkm::Id3((index % dims[0]) + extent.Min[0],
                   ((index / dims[0]) % dims[1]) + extent.Min[1],
                   ((index / (dims[0]*dims[1]))) + extent.Min[2]);
}

VTKM_EXEC_CONT_EXPORT
vtkm::Id2 ExtentCellFlatIndexToTopologyIndex(vtkm::Id index,
                                             const vtkm::Extent2 &extent)
{
  // Efficient implementation that tries to reduce the number of temporary
  // variables.
  const vtkm::Id2 dims = vtkm::ExtentCellDimensions(extent);
  return vtkm::Id2((index % dims[0]) + extent.Min[0],
                   (index / dims[0]) + extent.Min[1]);
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentPointTopologyIndexToFlatIndex(const vtkm::Vec<vtkm::Id,Dimensions> &ijk,
                                    const vtkm::Extent<Dimensions> &extent)
{
  const vtkm::Vec<vtkm::Id,Dimensions> dims = ExtentPointDimensions(extent);
  const vtkm::Vec<vtkm::Id,Dimensions> deltas = ijk - extent.Min;
  vtkm::Id flatIndex = deltas[Dimensions-1];
  for (vtkm::IdComponent dimIndex = Dimensions-2; dimIndex >= 0; dimIndex--)
  {
    flatIndex = flatIndex*dims[dimIndex] + deltas[dimIndex];
  }
  return flatIndex;
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentPointTopologyIndexToFlatIndex<1>(const vtkm::Vec<vtkm::Id,1> &ijk,
                                       const vtkm::Extent<1> &extent)
{
  return ijk[0] - extent.Min[0];
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentPointTopologyIndexToFlatIndex<2>(const vtkm::Vec<vtkm::Id,2> &ijk,
                                       const vtkm::Extent<2> &extent)
{
  const vtkm::Vec<vtkm::Id,2> dims = ExtentPointDimensions(extent);
  const vtkm::Vec<vtkm::Id,2> deltas = ijk - extent.Min;
  return (deltas[1] * dims[0] + deltas[0]);
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentPointTopologyIndexToFlatIndex<3>(const vtkm::Vec<vtkm::Id,3> &ijk,
                                       const vtkm::Extent<3> &extent)
{
  const vtkm::Vec<vtkm::Id,3> dims = ExtentPointDimensions(extent);
  const vtkm::Vec<vtkm::Id,3> deltas = ijk - extent.Min;
  return (deltas[2] * dims[1] + deltas[1]) * dims[0] + deltas[0];
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentCellTopologyIndexToFlatIndex(const vtkm::Vec<vtkm::Id,Dimensions> &ijk,
                                   const vtkm::Extent<Dimensions> &extent)
{
  const vtkm::Vec<vtkm::Id,Dimensions> dims = ExtentCellDimensions(extent);
  const vtkm::Vec<vtkm::Id,Dimensions> deltas = ijk - extent.Min;
  vtkm::Id flatIndex = deltas[Dimensions-1];
  for (vtkm::IdComponent dimIndex = Dimensions-2; dimIndex >= 0; dimIndex--)
  {
    flatIndex = flatIndex*dims[dimIndex] + deltas[dimIndex];
  }
  return flatIndex;
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentCellTopologyIndexToFlatIndex<1>(const vtkm::Vec<vtkm::Id,1> &ijk,
                                       const vtkm::Extent<1> &extent)
{
  return ijk[0] - extent.Min[0];
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentCellTopologyIndexToFlatIndex<2>(const vtkm::Vec<vtkm::Id,2> &ijk,
                                       const vtkm::Extent<2> &extent)
{
  const vtkm::Vec<vtkm::Id,2> dims = ExtentCellDimensions(extent);
  const vtkm::Vec<vtkm::Id,2> deltas = ijk - extent.Min;
  return (deltas[1] * dims[0] + deltas[0]);
}

/// Elements in structured grids have a single index with 0 being the entry at
/// the minimum extent in every direction and then increasing first in the r
/// then the s direction and so on. This method converts topological
/// coordinates to a flat index.
///
template<>
VTKM_EXEC_CONT_EXPORT
vtkm::Id
ExtentCellTopologyIndexToFlatIndex<3>(const vtkm::Vec<vtkm::Id,3> &ijk,
                                       const vtkm::Extent<3> &extent)
{
  const vtkm::Vec<vtkm::Id,3> dims = ExtentCellDimensions(extent);
  const vtkm::Vec<vtkm::Id,3> deltas = ijk - extent.Min;
  return (deltas[2] * dims[1] + deltas[1]) * dims[0] + deltas[0];
}

/// Given a cell index, returns the index to the first point incident on that
/// cell.
///
template<vtkm::IdComponent Dimensions>
VTKM_EXEC_CONT_EXPORT
vtkm::Id ExtentFirstPointOnCell(vtkm::Id cellIndex,
                                const Extent<Dimensions> &extent)
{
  return ExtentPointTopologyIndexToFlatIndex(
        ExtentCellFlatIndexToTopologyIndex(cellIndex,extent),
        extent);
}

} // namespace vtkm

#endif //vtk_m_Extent_h
