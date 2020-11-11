//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocatorChooser_h
#define vtk_m_cont_CellLocatorChooser_h

#include <vtkm/cont/CellLocatorRectilinearGrid.h>
#include <vtkm/cont/CellLocatorTwoLevel.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename CellSetType, typename CoordinateSystemArrayType>
struct CellLocatorChooserImpl
{
  using type = vtkm::cont::CellLocatorTwoLevel;
};

template <>
struct CellLocatorChooserImpl<vtkm::cont::CellSetStructured<3>,
                              vtkm::cont::ArrayHandleUniformPointCoordinates>
{
  using type = vtkm::cont::CellLocatorUniformGrid;
};

template <>
struct CellLocatorChooserImpl<
  vtkm::cont::CellSetStructured<3>,
  vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>
{
  using type = vtkm::cont::CellLocatorRectilinearGrid;
};

} // namespace detail

/// \brief A template to select an appropriate CellLocator based on CellSet type.
///
/// Given a concrete type for a `CellSet` subclass and a type of `ArrayHandle` for the
/// coordinate system, `CellLocatorChooser` picks an appropriate `CellLocator` for that
/// type of grid. It is a convenient class to use when you can resolve your templates
/// to discover the type of data set being used for location.
///
template <typename CellSetType, typename CoordinateSystemArrayType>
using CellLocatorChooser =
  typename detail::CellLocatorChooserImpl<CellSetType, CoordinateSystemArrayType>::type;

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellLocatorChooser_h
