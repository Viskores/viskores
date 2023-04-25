//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_ContourMarchingCells_h
#define vtk_m_filter_contour_ContourMarchingCells_h

#include <vtkm/filter/contour/AbstractContour.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief generate isosurface(s) from a Volume using the Marching Cells algorithm
///
/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
///
/// This implementation is not optimized for all use cases, it is used by
/// the more general \c Contour filter which selects the best implementation
/// for all types of \c DataSet . .
class VTKM_FILTER_CONTOUR_EXPORT ContourMarchingCells
  : public vtkm::filter::contour::AbstractContour
{
protected:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& result) override;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_ContourMarchingCells_h
