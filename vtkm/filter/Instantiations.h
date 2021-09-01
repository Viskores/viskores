//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Instantiations_h
#define vtk_m_Instantiations_h
//
// The following empty macros are instantiation delimiters used by
// vtk_add_instantiations at CMake/VTKmWrappers.cmake to generate transient
// instantiation files at the build directory.
//
// # Example #
//
// ## Contour.h ##
//
// VTKM_INSTANTIATION_BEGIN
// extern template vtkm::cont::DataSet Contour::DoExecute(
//   const vtkm::cont::DataSet&,
//   const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
//   const vtkm::filter::FieldMetadata&,
//   vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
// VTKM_INSTANTIATION_END
//
// ## ContourInstantiationsN.cxx ##
//
// template vtkm::cont::DataSet Contour::DoExecute(
//   const vtkm::cont::DataSet&,
//   const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
//   const vtkm::filter::FieldMetadata&,
//   vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
//
// # KNOWN ISSUES #
//
// Abstain to use the following constructors in the code section between
// the VTKM_INSTANTIATION_BEGIN/END directives:
//
// - The word extern other than for extern template.
// - The word _TEMPLATE_EXPORT other then for the EXPORT macro.
// - Comments that use the '$' symbol.
//
#define VTKM_INSTANTIATION_BEGIN
#define VTKM_INSTANTIATION_END

#endif
