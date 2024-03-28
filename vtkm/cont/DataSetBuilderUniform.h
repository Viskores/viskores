//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DataSetBuilderUniform_h
#define vtk_m_cont_DataSetBuilderUniform_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT DataSetBuilderUniform
{
  using VecType = vtkm::Vec3f;

public:
  VTKM_CONT
  DataSetBuilderUniform();

  /// @brief Create a 1D uniform `DataSet`.
  ///
  /// @param[in] dimension The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] origin The origin of the data. This is the point coordinate with
  ///   the minimum value in all dimensions.
  /// @param[in] spacing The uniform distance between adjacent points.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::Id& dimension,
                                              const T& origin,
                                              const T& spacing,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      vtkm::Id3(dimension, 1, 1),
      VecType(static_cast<vtkm::FloatDefault>(origin), 0, 0),
      VecType(static_cast<vtkm::FloatDefault>(spacing), 1, 1),
      coordNm);
  }

  /// @brief Create a 1D uniform `DataSet`.
  ///
  /// The origin is set to 0 and the spacing is set to 1.
  ///
  /// @param[in] dimension The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::Id& dimension,
                                              const std::string& coordNm = "coords");

  /// @brief Create a 2D uniform `DataSet`.
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] origin The origin of the data. This is the point coordinate with
  ///   the minimum value in all dimensions.
  /// @param[in] spacing The uniform distance between adjacent points.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::Id2& dimensions,
                                              const vtkm::Vec<T, 2>& origin,
                                              const vtkm::Vec<T, 2>& spacing,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(vtkm::Id3(dimensions[0], dimensions[1], 1),
                                                VecType(static_cast<vtkm::FloatDefault>(origin[0]),
                                                        static_cast<vtkm::FloatDefault>(origin[1]),
                                                        0),
                                                VecType(static_cast<vtkm::FloatDefault>(spacing[0]),
                                                        static_cast<vtkm::FloatDefault>(spacing[1]),
                                                        1),
                                                coordNm);
  }

  /// @brief Create a 2D uniform `DataSet`.
  ///
  /// The origin is set to (0,0) and the spacing is set to (1,1).
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::Id2& dimensions,
                                              const std::string& coordNm = "coords");

  /// @brief Create a 3D uniform `DataSet`.
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] origin The origin of the data. This is the point coordinate with
  ///   the minimum value in all dimensions.
  /// @param[in] spacing The uniform distance between adjacent points.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::Id3& dimensions,
                                              const vtkm::Vec<T, 3>& origin,
                                              const vtkm::Vec<T, 3>& spacing,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      vtkm::Id3(dimensions[0], dimensions[1], dimensions[2]),
      VecType(static_cast<vtkm::FloatDefault>(origin[0]),
              static_cast<vtkm::FloatDefault>(origin[1]),
              static_cast<vtkm::FloatDefault>(origin[2])),
      VecType(static_cast<vtkm::FloatDefault>(spacing[0]),
              static_cast<vtkm::FloatDefault>(spacing[1]),
              static_cast<vtkm::FloatDefault>(spacing[2])),
      coordNm);
  }

  /// @brief Create a 3D uniform `DataSet`.
  ///
  /// The origin is set to (0,0,0) and the spacing is set to (1,1,1).
  ///
  /// @param[in] dimensions The size of the grid. The dimensions are specified
  ///   based on the number of points (as opposed to the number of cells).
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::Id3& dimensions,
                                              const std::string& coordNm = "coords");

private:
  VTKM_CONT
  static vtkm::cont::DataSet CreateDataSet(const vtkm::Id3& dimensions,
                                           const vtkm::Vec3f& origin,
                                           const vtkm::Vec3f& spacing,
                                           const std::string& coordNm);
};

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_DataSetBuilderUniform_h
