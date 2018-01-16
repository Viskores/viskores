//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_ExtractStructured_h
#define vtk_m_worklet_ExtractStructured_h

#include <vtkm/RangeId3.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetListTag.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicCellSet.h>

namespace vtkm
{
namespace worklet
{

namespace extractstructured
{
namespace internal
{

class SubArrayPermutePoints
{
public:
  SubArrayPermutePoints() = default;

  SubArrayPermutePoints(vtkm::Id size,
                        vtkm::Id first,
                        vtkm::Id last,
                        vtkm::Id stride,
                        bool includeBoundary)
    : MaxIdx(size - 1)
    , First(first)
    , Last(last)
    , Stride(stride)
    , IncludeBoundary(includeBoundary)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id idx) const
  {
    return (this->IncludeBoundary && (idx == this->MaxIdx)) ? (this->Last)
                                                            : (this->First + (idx * this->Stride));
  }

private:
  vtkm::Id MaxIdx;
  vtkm::Id First, Last;
  vtkm::Id Stride;
  bool IncludeBoundary;
};

template <vtkm::IdComponent Dimensions>
class LogicalToFlatIndex;

template <>
class LogicalToFlatIndex<1>
{
public:
  LogicalToFlatIndex() = default;

  explicit LogicalToFlatIndex(const vtkm::Id3&) {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(const vtkm::Id3& index) const { return index[0]; }
};

template <>
class LogicalToFlatIndex<2>
{
public:
  LogicalToFlatIndex() = default;

  explicit LogicalToFlatIndex(const vtkm::Id3& dim)
    : XDim(dim[0])
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id operator()(const vtkm::Id3& index) const { return index[0] + index[1] * this->XDim; }

private:
  vtkm::Id XDim;
};

template <>
class LogicalToFlatIndex<3>
{
public:
  LogicalToFlatIndex() = default;

  explicit LogicalToFlatIndex(const vtkm::Id3& dim)
    : XDim(dim[0])
    , XYDim(dim[0] * dim[1])
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id operator()(const vtkm::Id3& index) const
  {
    return index[0] + index[1] * this->XDim + index[2] * this->XYDim;
  }

private:
  vtkm::Id XDim, XYDim;
};
}
} // extractstructured::internal

class ExtractStructured
{
public:
  using DynamicCellSetStructured =
    vtkm::cont::DynamicCellSetBase<vtkm::cont::CellSetListTagStructured>;

private:
  using AxisIndexArrayPoints =
    vtkm::cont::ArrayHandleImplicit<extractstructured::internal::SubArrayPermutePoints>;
  using PointIndexArray = vtkm::cont::ArrayHandleCartesianProduct<AxisIndexArrayPoints,
                                                                  AxisIndexArrayPoints,
                                                                  AxisIndexArrayPoints>;

  using AxisIndexArrayCells = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
  using CellIndexArray = vtkm::cont::ArrayHandleCartesianProduct<AxisIndexArrayCells,
                                                                 AxisIndexArrayCells,
                                                                 AxisIndexArrayCells>;

  static AxisIndexArrayPoints MakeAxisIndexArrayPoints(vtkm::Id count,
                                                       vtkm::Id first,
                                                       vtkm::Id last,
                                                       vtkm::Id stride,
                                                       bool includeBoundary)
  {
    auto fnctr = extractstructured::internal::SubArrayPermutePoints(
      count, first, last, stride, includeBoundary);
    return vtkm::cont::make_ArrayHandleImplicit(fnctr, count);
  }

  static AxisIndexArrayCells MakeAxisIndexArrayCells(vtkm::Id count,
                                                     vtkm::Id start,
                                                     vtkm::Id stride)
  {
    return vtkm::cont::make_ArrayHandleCounting(start, stride, count);
  }

  static DynamicCellSetStructured MakeCellSetStructured(const vtkm::Id3& dimensions)
  {
    int dimensionality = 0;
    vtkm::Id xyz[3];
    for (int i = 0; i < 3; ++i)
    {
      if (dimensions[i] > 1)
      {
        xyz[dimensionality++] = dimensions[i];
      }
    }
    switch (dimensionality)
    {
      case 1:
      {
        vtkm::cont::CellSetStructured<1> outCs;
        outCs.SetPointDimensions(xyz[0]);
        return outCs;
      }
      case 2:
      {
        vtkm::cont::CellSetStructured<2> outCs;
        outCs.SetPointDimensions(vtkm::Id2(xyz[0], xyz[1]));
        return outCs;
      }
      case 3:
      {
        vtkm::cont::CellSetStructured<3> outCs;
        outCs.SetPointDimensions(vtkm::Id3(xyz[0], xyz[1], xyz[2]));
        return outCs;
      }
      default:
        return DynamicCellSetStructured();
    }
  }

public:
  template <vtkm::IdComponent Dimensionality, typename DeviceAdapter>
  DynamicCellSetStructured Run(const vtkm::cont::CellSetStructured<Dimensionality>& cellset,
                               const vtkm::RangeId3& voi,
                               const vtkm::Id3& sampleRate,
                               bool includeBoundary,
                               DeviceAdapter)
  {
    // Verify input parameters
    vtkm::Vec<vtkm::Id, Dimensionality> ptdim(cellset.GetPointDimensions());
    switch (Dimensionality)
    {
      case 1:
      {
        if (sampleRate[0] < 1)
        {
          throw vtkm::cont::ErrorBadValue("Bad sampling rate");
        }
        this->SampleRate = vtkm::Id3(sampleRate[0], 1, 1);
        this->InputDimensions = vtkm::Id3(ptdim[0], 1, 1);
        break;
      }
      case 2:
      {
        if (sampleRate[0] < 1 || sampleRate[1] < 1)
        {
          throw vtkm::cont::ErrorBadValue("Bad sampling rate");
        }
        this->SampleRate = vtkm::Id3(sampleRate[0], sampleRate[1], 1);
        this->InputDimensions = vtkm::Id3(ptdim[0], ptdim[1], 1);
        break;
      }
      case 3:
      {
        if (sampleRate[0] < 1 || sampleRate[1] < 1 || sampleRate[2] < 1)
        {
          throw vtkm::cont::ErrorBadValue("Bad sampling rate");
        }
        this->SampleRate = sampleRate;
        this->InputDimensions = vtkm::Id3(ptdim[0], ptdim[1], ptdim[2]);
        break;
      }
      default:
        VTKM_ASSERT(false && "Unsupported number of dimensions");
    }
    this->InputDimensionality = Dimensionality;

    // intersect VOI
    this->VOI.X.Min = vtkm::Max(vtkm::Id(0), voi.X.Min);
    this->VOI.X.Max = vtkm::Min(this->InputDimensions[0], voi.X.Max);
    this->VOI.Y.Min = vtkm::Max(vtkm::Id(0), voi.Y.Min);
    this->VOI.Y.Max = vtkm::Min(this->InputDimensions[1], voi.Y.Max);
    this->VOI.Z.Min = vtkm::Max(vtkm::Id(0), voi.Z.Min);
    this->VOI.Z.Max = vtkm::Min(this->InputDimensions[2], voi.Z.Max);
    if (!this->VOI.IsNonEmpty()) // empty VOI
    {
      return DynamicCellSetStructured();
    }

    // compute output dimensions
    this->OutputDimensions = vtkm::Id3(1);
    vtkm::Id3 voiDims = this->VOI.Dimensions();
    for (int i = 0; i < Dimensionality; ++i)
    {
      this->OutputDimensions[i] = ((voiDims[i] + this->SampleRate[i] - 1) / this->SampleRate[i]) +
        ((includeBoundary && ((voiDims[i] - 1) % this->SampleRate[i])) ? 1 : 0);
    }

    this->ValidPoints = vtkm::cont::make_ArrayHandleCartesianProduct(
      MakeAxisIndexArrayPoints(this->OutputDimensions[0],
                               this->VOI.X.Min,
                               this->VOI.X.Max - 1,
                               this->SampleRate[0],
                               includeBoundary),
      MakeAxisIndexArrayPoints(this->OutputDimensions[1],
                               this->VOI.Y.Min,
                               this->VOI.Y.Max - 1,
                               this->SampleRate[1],
                               includeBoundary),
      MakeAxisIndexArrayPoints(this->OutputDimensions[2],
                               this->VOI.Z.Min,
                               this->VOI.Z.Max - 1,
                               this->SampleRate[2],
                               includeBoundary));

    this->ValidCells = vtkm::cont::make_ArrayHandleCartesianProduct(
      MakeAxisIndexArrayCells(vtkm::Max(vtkm::Id(1), this->OutputDimensions[0] - 1),
                              this->VOI.X.Min,
                              this->SampleRate[0]),
      MakeAxisIndexArrayCells(vtkm::Max(vtkm::Id(1), this->OutputDimensions[1] - 1),
                              this->VOI.Y.Min,
                              this->SampleRate[1]),
      MakeAxisIndexArrayCells(vtkm::Max(vtkm::Id(1), this->OutputDimensions[2] - 1),
                              this->VOI.Z.Min,
                              this->SampleRate[2]));

    return MakeCellSetStructured(this->OutputDimensions);
  }

private:
  template <typename DeviceAdapter>
  class CallRun
  {
  public:
    CallRun(ExtractStructured* worklet,
            const vtkm::RangeId3& voi,
            const vtkm::Id3& sampleRate,
            bool includeBoundary,
            DynamicCellSetStructured& output)
      : Worklet(worklet)
      , VOI(&voi)
      , SampleRate(&sampleRate)
      , IncludeBoundary(includeBoundary)
      , Output(&output)
    {
    }

    template <vtkm::IdComponent Dimensionality>
    void operator()(const vtkm::cont::CellSetStructured<Dimensionality>& cellset) const
    {
      *this->Output = this->Worklet->Run(
        cellset, *this->VOI, *this->SampleRate, this->IncludeBoundary, DeviceAdapter());
    }

    template <typename CellSetType>
    void operator()(const CellSetType&) const
    {
      throw vtkm::cont::ErrorBadType("ExtractStructured only works with structured datasets");
    }

  private:
    ExtractStructured* Worklet;
    const vtkm::RangeId3* VOI;
    const vtkm::Id3* SampleRate;
    bool IncludeBoundary;
    DynamicCellSetStructured* Output;
  };

public:
  template <typename CellSetList, typename DeviceAdapter>
  DynamicCellSetStructured Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset,
                               const vtkm::RangeId3& voi,
                               const vtkm::Id3& sampleRate,
                               bool includeBoundary,
                               DeviceAdapter)
  {
    DynamicCellSetStructured output;
    CallRun<DeviceAdapter> cr(this, voi, sampleRate, includeBoundary, output);
    vtkm::cont::CastAndCall(cellset, cr);
    return output;
  }

private:
  template <typename DeviceAdapter>
  class CoordinatesMapper
  {
  private:
    using UniformCoordinatesArrayHandle =
      vtkm::cont::ArrayHandleUniformPointCoordinates::Superclass;

    template <typename T, typename Storage1, typename Storage2, typename Storage3>
    using RectilinearCoordinatesArrayHandle = typename vtkm::cont::ArrayHandleCartesianProduct<
      vtkm::cont::ArrayHandle<T, Storage1>,
      vtkm::cont::ArrayHandle<T, Storage2>,
      vtkm::cont::ArrayHandle<T, Storage3>>::Superclass;

  public:
    CoordinatesMapper(const ExtractStructured* worklet,
                      vtkm::cont::ArrayHandleVirtualCoordinates& result)
      : Worklet(worklet)
      , Result(&result)
    {
    }

    void operator()(const UniformCoordinatesArrayHandle& coords) const
    {
      using CoordsArray = vtkm::cont::ArrayHandleUniformPointCoordinates;
      using CoordType = CoordsArray::ValueType;
      using ValueType = CoordType::ComponentType;

      const auto& portal = coords.GetPortalConstControl();
      CoordType inOrigin = portal.GetOrigin();
      CoordType inSpacing = portal.GetSpacing();

      CoordType outOrigin = vtkm::make_Vec(
        inOrigin[0] + static_cast<ValueType>(this->Worklet->VOI.X.Min) * inSpacing[0],
        inOrigin[1] + static_cast<ValueType>(this->Worklet->VOI.Y.Min) * inSpacing[1],
        inOrigin[2] + static_cast<ValueType>(this->Worklet->VOI.Z.Min) * inSpacing[2]);
      CoordType outSpacing = inSpacing * static_cast<CoordType>(this->Worklet->SampleRate);

      auto out = CoordsArray(this->Worklet->OutputDimensions, outOrigin, outSpacing);
      *this->Result = vtkm::cont::ArrayHandleVirtualCoordinates(out);
    }

    template <typename T, typename Storage1, typename Storage2, typename Storage3>
    void operator()(
      const RectilinearCoordinatesArrayHandle<T, Storage1, Storage2, Storage3>& coords) const
    {
      vtkm::cont::ArrayHandle<T> xs, ys, zs;

      // For structured datasets, the cellsets are of different types based on
      // its dimensionality, but the coordinates are always 3 dimensional.
      // We can map the axis of the cellset to the coordinates by looking at the
      // length of a coordinate axis array.
      const AxisIndexArrayPoints* validIds[3] = {
        &this->Worklet->ValidPoints.GetStorage().GetFirstArray(),
        &this->Worklet->ValidPoints.GetStorage().GetSecondArray(),
        &this->Worklet->ValidPoints.GetStorage().GetThirdArray()
      };

      int dim = 0;
      dim += RectilinearCoordsCopy(coords.GetStorage().GetFirstArray(), *validIds[dim], xs);
      dim += RectilinearCoordsCopy(coords.GetStorage().GetSecondArray(), *validIds[dim], ys);
      dim += RectilinearCoordsCopy(coords.GetStorage().GetThirdArray(), *validIds[dim], zs);
      VTKM_ASSERT(dim == this->Worklet->InputDimensionality);

      auto out = vtkm::cont::make_ArrayHandleCartesianProduct(xs, ys, zs);
      *this->Result = vtkm::cont::ArrayHandleVirtualCoordinates(out);
    }

    template <typename T, typename Storage>
    void operator()(const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, Storage>& coords) const
    {
      auto out = this->Worklet->ProcessPointField(coords, DeviceAdapter());
      *this->Result = vtkm::cont::ArrayHandleVirtualCoordinates(out);
    }

  private:
    template <typename T, typename Storage>
    static int RectilinearCoordsCopy(const vtkm::cont::ArrayHandle<T, Storage>& coords,
                                     const AxisIndexArrayPoints& valid,
                                     vtkm::cont::ArrayHandle<T>& dest)
    {
      if (coords.GetNumberOfValues() == 1)
      {
        dest.GetPortalControl().Set(0, coords.GetPortalConstControl().Get(0));
        return 0;
      }
      else
      {
        vtkm::cont::ArrayCopy(
          vtkm::cont::make_ArrayHandlePermutation(valid, coords), dest, DeviceAdapter());
        return 1;
      }
    }

    const ExtractStructured* Worklet;
    vtkm::cont::ArrayHandleVirtualCoordinates* Result;
  };

public:
  template <typename DeviceAdapter>
  vtkm::cont::ArrayHandleVirtualCoordinates MapCoordinates(
    const vtkm::cont::CoordinateSystem& coordinates,
    DeviceAdapter)
  {
    vtkm::cont::ArrayHandleVirtualCoordinates result;
    CoordinatesMapper<DeviceAdapter> mapper(this, result);
    vtkm::cont::CastAndCall(coordinates, mapper);
    return result;
  }

private:
  template <vtkm::IdComponent Dimensionality, typename T, typename Storage, typename DeviceAdapter>
  void MapPointField(const vtkm::cont::ArrayHandle<T, Storage>& in,
                     vtkm::cont::ArrayHandle<T>& out,
                     DeviceAdapter) const
  {
    using namespace extractstructured::internal;
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    auto validPointsFlat = vtkm::cont::make_ArrayHandleTransform(
      this->ValidPoints, LogicalToFlatIndex<Dimensionality>(this->InputDimensions));
    Algorithm::Copy(make_ArrayHandlePermutation(validPointsFlat, in), out);
  }

  template <vtkm::IdComponent Dimensionality, typename T, typename Storage, typename DeviceAdapter>
  void MapCellField(const vtkm::cont::ArrayHandle<T, Storage>& in,
                    vtkm::cont::ArrayHandle<T>& out,
                    DeviceAdapter) const
  {
    using namespace extractstructured::internal;
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    auto inputCellDimensions = this->InputDimensions - vtkm::Id3(1);
    auto validCellsFlat = vtkm::cont::make_ArrayHandleTransform(
      this->ValidCells, LogicalToFlatIndex<Dimensionality>(inputCellDimensions));
    Algorithm::Copy(make_ArrayHandlePermutation(validCellsFlat, in), out);
  }

public:
  template <typename T, typename Storage, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<T> ProcessPointField(const vtkm::cont::ArrayHandle<T, Storage>& field,
                                               DeviceAdapter device) const
  {
    vtkm::cont::ArrayHandle<T> result;
    switch (this->InputDimensionality)
    {
      case 1:
        this->MapPointField<1>(field, result, device);
        break;
      case 2:
        this->MapPointField<2>(field, result, device);
        break;
      case 3:
        this->MapPointField<3>(field, result, device);
        break;
      default:
        break;
    }

    return result;
  }

  template <typename T, typename Storage, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<T> ProcessCellField(const vtkm::cont::ArrayHandle<T, Storage>& field,
                                              DeviceAdapter device) const
  {
    vtkm::cont::ArrayHandle<T> result;
    switch (this->InputDimensionality)
    {
      case 1:
        this->MapCellField<1>(field, result, device);
        break;
      case 2:
        this->MapCellField<2>(field, result, device);
        break;
      case 3:
        this->MapCellField<3>(field, result, device);
        break;
      default:
        break;
    }

    return result;
  }

private:
  vtkm::RangeId3 VOI;
  vtkm::Id3 SampleRate;

  int InputDimensionality;
  vtkm::Id3 InputDimensions;
  vtkm::Id3 OutputDimensions;

  PointIndexArray ValidPoints;
  CellIndexArray ValidCells;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_ExtractStructured_h
