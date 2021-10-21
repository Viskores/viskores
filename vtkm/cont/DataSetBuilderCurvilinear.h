//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DataSetBuilderCurvilinear_h
#define vtk_m_cont_DataSetBuilderCurvilinear_h

#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT DataSetBuilderCurvilinear
{
public:
  VTKM_CONT
  DataSetBuilderCurvilinear();

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::string& coordsNm = "coords")
  {
    VTKM_ASSERT(xVals.size() > 0);

    std::vector<T> yVals(xVals.size(), 0), zVals(xVals.size(), 0);
    vtkm::Id dim = static_cast<vtkm::Id>(xVals.size());
    auto coords = vtkm::cont::make_ArrayHandleSOA<vtkm::Vec<T, 3>>({ xVals, yVals, zVals });

    return DataSetBuilderCurvilinear::Create(coords, { dim, 0, 0 }, 1, coordsNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const vtkm::Id2& dims,
                                              const std::string& coordsNm = "coords")
  {
    VTKM_ASSERT(xVals.size() > 0 && xVals.size() == yVals.size());

    std::vector<T> zVals(xVals.size(), 0);
    auto coords = vtkm::cont::make_ArrayHandleSOA<vtkm::Vec<T, 3>>({ xVals, yVals, zVals });

    return DataSetBuilderCurvilinear::Create(coords, { dims[0], dims[1], 0 }, 2, coordsNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<T>& zVals,
                                              const vtkm::Id3& dims,
                                              const std::string& coordsNm = "coords")
  {
    VTKM_ASSERT(xVals.size() > 0 && xVals.size() == yVals.size());
    VTKM_ASSERT(xVals.size() == zVals.size());

    auto coords = vtkm::cont::make_ArrayHandleSOA<vtkm::Vec<T, 3>>({ xVals, yVals, zVals });

    return DataSetBuilderCurvilinear::Create(coords, dims, 3, coordsNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& points,
                                              const vtkm::Id3& dims,
                                              const std::string& coordsNm = "coords")
  {
    auto coords = vtkm::cont::make_ArrayHandle(points);
    return DataSetBuilderCurvilinear::Create(coords, dims, 3, coordsNm);
  }

  template <typename CoordsType>
  VTKM_CONT static vtkm::cont::DataSet Create(const CoordsType& coords,
                                              const vtkm::Id3& dims,
                                              const std::string& coordsNm = "coords")
  {
    return DataSetBuilderCurvilinear::Create(coords, dims, 3, coordsNm);
  }

  template <typename CoordsType>
  VTKM_CONT static vtkm::cont::DataSet Create(const CoordsType& coords,
                                              const vtkm::Id2& dims,
                                              const std::string& coordsNm = "coords")
  {
    return DataSetBuilderCurvilinear::Create(coords, { dims[0], dims[1], 0 }, 2, coordsNm);
  }

  template <typename CoordsType>
  VTKM_CONT static vtkm::cont::DataSet Create(const CoordsType& coords,
                                              const std::string& coordsNm = "coords")
  {
    return DataSetBuilderCurvilinear::Create(
      coords, { coords.GetNumberOfValues(), 0, 0 }, 1, coordsNm);
  }

private:
  template <typename CoordsType>
  VTKM_CONT static vtkm::cont::DataSet Create(const CoordsType& coords,
                                              const vtkm::Id3& dims,
                                              const vtkm::Id& cellSetDim,
                                              const std::string& coordsNm = "coords")
  {
    vtkm::cont::DataSet ds;
    ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm, coords));

    if (cellSetDim == 3)
    {
      VTKM_ASSERT(dims[0] >= 1 && dims[1] >= 1 && dims[2] >= 1);
      VTKM_ASSERT(coords.GetNumberOfValues() == dims[0] * dims[1] * dims[2]);

      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(dims);
      ds.SetCellSet(cellSet);
    }
    else if (cellSetDim == 2)
    {
      VTKM_ASSERT(dims[0] >= 1 && dims[1] >= 1 && dims[2] == 0);
      VTKM_ASSERT(coords.GetNumberOfValues() == dims[0] * dims[1]);

      vtkm::cont::CellSetStructured<2> cellSet;
      cellSet.SetPointDimensions({ dims[0], dims[1] });
      ds.SetCellSet(cellSet);
    }
    else if (cellSetDim == 1)
    {
      VTKM_ASSERT(dims[0] >= 1 && dims[1] == 0 && dims[2] == 0);
      VTKM_ASSERT(coords.GetNumberOfValues() == dims[0]);

      vtkm::cont::CellSetStructured<1> cellSet;
      cellSet.SetPointDimensions(dims[0]);
      ds.SetCellSet(cellSet);
    }
    else
      throw vtkm::cont::ErrorBadValue("Unsupported CellSetStructured dimension.");

    return ds;
  }
};

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_DataSetBuilderCurvilinear_h
