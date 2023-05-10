//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_ContourFlyingEdges_h
#define vtk_m_filter_contour_ContourFlyingEdges_h

#include <vtkm/filter/contour/AbstractContour.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief generate isosurface(s) from a 3-dimensional structured mesh

/// Takes as input a 3D structured mesh and generates on
/// output one or more isosurfaces using the Flying Edges algorithm.
/// Multiple contour values must be specified to generate the isosurfaces.
///
/// This implementation only accepts \c CellSetStructured<3> inputs using
/// \c ArrayHandleUniformPointCoordinates for point coordinates,
/// and is only used as part of the more general \c Contour filter
class VTKM_FILTER_CONTOUR_EXPORT ContourFlyingEdges : public vtkm::filter::contour::AbstractContour
{
protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& result) override;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_ContourFlyingEdges_h
