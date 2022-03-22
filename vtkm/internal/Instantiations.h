//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_internal_Instantiations_h
#define vtk_m_internal_Instantiations_h
///
/// The following empty macros are instantiation delimiters used by
/// `vtk_add_instantiations` at CMake/VTKmWrappers.cmake to generate transient
/// instantiation files at the build directory.
///
/// # Example #
///
/// ```cpp
/// VTKM_INSTANTIATION_BEGIN
/// extern template vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec3f_32, 3>>
/// vtkm::worklet::CellGradient::Run(
///   const vtkm::cont::UnknownCellSet&,
///   const vtkm::cont::CoordinateSystem&,
///   const vtkm::cont::ArrayHandle<vtkm::Vec3f_32, vtkm::cont::StorageTagSOA>&,
///   GradientOutputFields<vtkm::Vec3f_32>&);
/// VTKM_INSTANTIATION_END
/// ```
///
/// # KNOWN ISSUES #
///
/// Abstain to use the following constructors in the code section between
/// the VTKM_INSTANTIATION_BEGIN/END directives:
///
/// - The word extern other than for extern template.
/// - The word _TEMPLATE_EXPORT other then for the EXPORT macro.
/// - Comments that use the '$' symbol.
/// - Symbols for functions or methods that are inline. This includes methods
///   with implementation defined in the class/struct definition.
///
/// # See Also #
///
/// See the documentation for the `vtkm_add_instantiations` function in
/// CMake/VTKmWrappers.cmake for more information.
///
#define VTKM_INSTANTIATION_BEGIN
#define VTKM_INSTANTIATION_END

#endif //vtk_m_internal_Instantiations_h
