//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DataSetBuilderRectilinear_h
#define vtk_m_cont_DataSetBuilderRectilinear_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT DataSetBuilderRectilinear
{
  template <typename T, typename U>
  VTKM_CONT static void CopyInto(const std::vector<T>& input, vtkm::cont::ArrayHandle<U>& output)
  {
    DataSetBuilderRectilinear::CopyInto(vtkm::cont::make_ArrayHandle(input, vtkm::CopyFlag::Off),
                                        output);
  }

  template <typename T, typename U>
  VTKM_CONT static void CopyInto(const vtkm::cont::ArrayHandle<T>& input,
                                 vtkm::cont::ArrayHandle<U>& output)
  {
    vtkm::cont::ArrayCopy(input, output);
  }

  template <typename T, typename U>
  VTKM_CONT static void CopyInto(const T* input, vtkm::Id len, vtkm::cont::ArrayHandle<U>& output)
  {
    DataSetBuilderRectilinear::CopyInto(
      vtkm::cont::make_ArrayHandle(input, len, vtkm::CopyFlag::Off), output);
  }

public:
  VTKM_CONT
  DataSetBuilderRectilinear();

  /// @brief Create a 1D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with a scalar array for the point coordinates
  /// in the x direction.
  /// In this form, the coordinate array is specified with `std::vector`.
  /// The data is copied from the `std::vector`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xvals,
                                              const std::string& coordNm = "coords")
  {
    std::vector<T> yvals(1, 0), zvals(1, 0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 1D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with a scalar array for the point coordinates
  /// in the x direction.
  /// In this form, the coordinate array is specified with a standard C array.
  /// The data is copied from the array.
  ///
  /// @param[in] nx The size of the grid in the x direction (and length of the @a xvals array).
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(vtkm::Id nx,
                                              T* xvals,
                                              const std::string& coordNm = "coords")
  {
    T yvals = 0, zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(nx, 1, 1, xvals, &yvals, &zvals, coordNm);
  }

  /// @brief Create a 1D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with a scalar array for the point coordinates
  /// in the x direction.
  /// In this form, the coordinate array is specified with `vtkm::cont::ArrayHandle`.
  /// The `ArrayHandle` is shared with the `DataSet`, so changing the `ArrayHandle`
  /// changes the `DataSet`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::cont::ArrayHandle<T>& xvals,
                                              const std::string& coordNm = "coords")
  {
    vtkm::cont::ArrayHandle<T> yvals, zvals;
    yvals.Allocate(1);
    yvals.WritePortal().Set(0, 0.0);
    zvals.Allocate(1);
    zvals.WritePortal().Set(0, 0.0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 2D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x and y directions.
  /// In this form, the coordinate arrays are specified with `std::vector`.
  /// The data is copied from the `std::vector`s.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xvals,
                                              const std::vector<T>& yvals,
                                              const std::string& coordNm = "coords")
  {
    std::vector<T> zvals(1, 0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 2D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x and y directions.
  /// In this form, the coordinate arrays are specified with standard C arrays.
  /// The data is copied from the arrays.
  ///
  /// @param[in] nx The size of the grid in the x direction (and length of the @a xvals array).
  /// @param[in] ny The size of the grid in the x direction (and length of the @a yvals array).
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(vtkm::Id nx,
                                              vtkm::Id ny,
                                              T* xvals,
                                              T* yvals,
                                              const std::string& coordNm = "coords")
  {
    T zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(nx, ny, 1, xvals, yvals, &zvals, coordNm);
  }

  /// @brief Create a 2D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x and y directions.
  /// In this form, the coordinate arrays are specified with `vtkm::cont::ArrayHandle`.
  /// The `ArrayHandle`s are shared with the `DataSet`, so changing the `ArrayHandle`s
  /// changes the `DataSet`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::cont::ArrayHandle<T>& xvals,
                                              const vtkm::cont::ArrayHandle<T>& yvals,
                                              const std::string& coordNm = "coords")
  {
    vtkm::cont::ArrayHandle<T> zvals;
    zvals.Allocate(1);
    zvals.WritePortal().Set(0, 0.0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 3D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x, y, and z directions.
  /// In this form, the coordinate arrays are specified with standard C arrays.
  /// The data is copied from the arrays.
  ///
  /// @param[in] nx The size of the grid in the x direction (and length of the @a xvals array).
  /// @param[in] ny The size of the grid in the x direction (and length of the @a yvals array).
  /// @param[in] nz The size of the grid in the x direction (and length of the @a zvals array).
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] zvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(vtkm::Id nx,
                                              vtkm::Id ny,
                                              vtkm::Id nz,
                                              T* xvals,
                                              T* yvals,
                                              T* zvals,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(nx, ny, nz, xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 3D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x, y, and z directions.
  /// In this form, the coordinate arrays are specified with `std::vector`.
  /// The data is copied from the `std::vector`s.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] zvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xvals,
                                              const std::vector<T>& yvals,
                                              const std::vector<T>& zvals,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 3D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x, y, and z directions.
  /// In this form, the coordinate arrays are specified with `vtkm::cont::ArrayHandle`.
  /// The `ArrayHandle`s are shared with the `DataSet`, so changing the `ArrayHandle`s
  /// changes the `DataSet`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] zvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const vtkm::cont::ArrayHandle<T>& xvals,
                                              const vtkm::cont::ArrayHandle<T>& yvals,
                                              const vtkm::cont::ArrayHandle<T>& zvals,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

private:
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(const std::vector<T>& xvals,
                                                    const std::vector<T>& yvals,
                                                    const std::vector<T>& zvals,
                                                    const std::string& coordNm)
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(Xc, Yc, Zc, coordNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(vtkm::Id nx,
                                                    vtkm::Id ny,
                                                    vtkm::Id nz,
                                                    const T* xvals,
                                                    const T* yvals,
                                                    const T* zvals,
                                                    const std::string& coordNm)
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, nx, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, ny, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, nz, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(Xc, Yc, Zc, coordNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(const vtkm::cont::ArrayHandle<T>& X,
                                                    const vtkm::cont::ArrayHandle<T>& Y,
                                                    const vtkm::cont::ArrayHandle<T>& Z,
                                                    const std::string& coordNm)
  {
    vtkm::cont::DataSet dataSet;

    //Convert all coordinates to floatDefault.
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>
      coords;

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(X, Xc);
    DataSetBuilderRectilinear::CopyInto(Y, Yc);
    DataSetBuilderRectilinear::CopyInto(Z, Zc);

    coords = vtkm::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
    vtkm::cont::CoordinateSystem cs(coordNm, coords);
    dataSet.AddCoordinateSystem(cs);

    // compute the dimensions of the cellset by counting the number of axes
    // with >1 dimension
    int ndims = 0;
    vtkm::Id dims[3];
    if (Xc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Xc.GetNumberOfValues();
    }
    if (Yc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Yc.GetNumberOfValues();
    }
    if (Zc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Zc.GetNumberOfValues();
    }

    if (ndims == 1)
    {
      vtkm::cont::CellSetStructured<1> cellSet;
      cellSet.SetPointDimensions(dims[0]);
      dataSet.SetCellSet(cellSet);
    }
    else if (ndims == 2)
    {
      vtkm::cont::CellSetStructured<2> cellSet;
      cellSet.SetPointDimensions(vtkm::make_Vec(dims[0], dims[1]));
      dataSet.SetCellSet(cellSet);
    }
    else if (ndims == 3)
    {
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::make_Vec(dims[0], dims[1], dims[2]));
      dataSet.SetCellSet(cellSet);
    }
    else
    {
      throw vtkm::cont::ErrorBadValue("Invalid cell set dimension");
    }

    return dataSet;
  }
};

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_DataSetBuilderRectilinear_h
