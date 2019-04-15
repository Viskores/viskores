//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
#include <vtkm/cont/CoordinateSystem.h>
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
  template <vtkm::IdComponent Dimensionality>
  DynamicCellSetStructured Run(const vtkm::cont::CellSetStructured<Dimensionality>& cellset,
                               const vtkm::RangeId3& voi,
                               const vtkm::Id3& sampleRate,
                               bool includeBoundary)
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
      *this->Output =
        this->Worklet->Run(cellset, *this->VOI, *this->SampleRate, this->IncludeBoundary);
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
  template <typename CellSetList>
  DynamicCellSetStructured Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset,
                               const vtkm::RangeId3& voi,
                               const vtkm::Id3& sampleRate,
                               bool includeBoundary)
  {
    DynamicCellSetStructured output;
    CallRun cr(this, voi, sampleRate, includeBoundary, output);
    vtkm::cont::CastAndCall(cellset, cr);
    return output;
  }

private:
  using UniformCoordinatesArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates::Superclass;

  using RectilinearCoordinatesArrayHandle = vtkm::cont::ArrayHandleCartesianProduct<
    vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
    vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
    vtkm::cont::ArrayHandle<vtkm::FloatDefault>>::Superclass;


  vtkm::cont::ArrayHandleVirtualCoordinates MapCoordinatesUniform(
    const UniformCoordinatesArrayHandle& coords)
  {
    using CoordsArray = vtkm::cont::ArrayHandleUniformPointCoordinates;
    using CoordType = CoordsArray::ValueType;
    using ValueType = CoordType::ComponentType;

    const auto& portal = coords.GetPortalConstControl();
    CoordType inOrigin = portal.GetOrigin();
    CoordType inSpacing = portal.GetSpacing();

    CoordType outOrigin =
      vtkm::make_Vec(inOrigin[0] + static_cast<ValueType>(this->VOI.X.Min) * inSpacing[0],
                     inOrigin[1] + static_cast<ValueType>(this->VOI.Y.Min) * inSpacing[1],
                     inOrigin[2] + static_cast<ValueType>(this->VOI.Z.Min) * inSpacing[2]);
    CoordType outSpacing = inSpacing * static_cast<CoordType>(this->SampleRate);

    auto out = CoordsArray(this->OutputDimensions, outOrigin, outSpacing);
    return vtkm::cont::ArrayHandleVirtualCoordinates(out);
  }

  vtkm::cont::ArrayHandleVirtualCoordinates MapCoordinatesRectilinear(
    const RectilinearCoordinatesArrayHandle& coords)
  {
    // For structured datasets, the cellsets are of different types based on
    // its dimensionality, but the coordinates are always 3 dimensional.
    // We can map the axis of the cellset to the coordinates by looking at the
    // length of a coordinate axis array.
    AxisIndexArrayPoints validIds[3] = { this->ValidPoints.GetStorage().GetFirstArray(),
                                         this->ValidPoints.GetStorage().GetSecondArray(),
                                         this->ValidPoints.GetStorage().GetThirdArray() };

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> arrays[3] = { coords.GetStorage().GetFirstArray(),
                                                              coords.GetStorage().GetSecondArray(),
                                                              coords.GetStorage().GetThirdArray() };

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> xyzs[3];
    int dim = 0;
    for (int i = 0; i < 3; ++i)
    {
      if (arrays[i].GetNumberOfValues() == 1)
      {
        xyzs[i].Allocate(1);
        xyzs[i].GetPortalControl().Set(0, arrays[i].GetPortalConstControl().Get(0));
      }
      else
      {
        vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandlePermutation(validIds[i], arrays[i]),
                              xyzs[i]);
        ++dim;
      }
    }
    VTKM_ASSERT(dim == this->InputDimensionality);

    auto out = vtkm::cont::make_ArrayHandleCartesianProduct(xyzs[0], xyzs[1], xyzs[2]);
    return vtkm::cont::ArrayHandleVirtualCoordinates(out);
  }

public:
  vtkm::cont::ArrayHandleVirtualCoordinates MapCoordinates(
    const vtkm::cont::CoordinateSystem& coordinates)
  {
    auto coArray = coordinates.GetData();
    if (coArray.IsType<UniformCoordinatesArrayHandle>())
    {
      return this->MapCoordinatesUniform(coArray.Cast<UniformCoordinatesArrayHandle>());
    }
    else if (coArray.IsType<RectilinearCoordinatesArrayHandle>())
    {
      return this->MapCoordinatesRectilinear(coArray.Cast<RectilinearCoordinatesArrayHandle>());
    }
    else
    {
      auto out = this->ProcessPointField(coArray);
      return vtkm::cont::ArrayHandleVirtualCoordinates(out);
    }
  }

private:
  template <vtkm::IdComponent Dimensionality, typename T, typename Storage>
  void MapPointField(const vtkm::cont::ArrayHandle<T, Storage>& in,
                     vtkm::cont::ArrayHandle<T>& out) const
  {
    using namespace extractstructured::internal;

    auto validPointsFlat = vtkm::cont::make_ArrayHandleTransform(
      this->ValidPoints, LogicalToFlatIndex<Dimensionality>(this->InputDimensions));
    vtkm::cont::ArrayCopy(make_ArrayHandlePermutation(validPointsFlat, in), out);
  }

  template <vtkm::IdComponent Dimensionality, typename T, typename Storage>
  void MapCellField(const vtkm::cont::ArrayHandle<T, Storage>& in,
                    vtkm::cont::ArrayHandle<T>& out) const
  {
    using namespace extractstructured::internal;

    auto inputCellDimensions = this->InputDimensions - vtkm::Id3(1);
    auto validCellsFlat = vtkm::cont::make_ArrayHandleTransform(
      this->ValidCells, LogicalToFlatIndex<Dimensionality>(inputCellDimensions));
    vtkm::cont::ArrayCopy(make_ArrayHandlePermutation(validCellsFlat, in), out);
  }

public:
  template <typename T, typename Storage>
  vtkm::cont::ArrayHandle<T> ProcessPointField(
    const vtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    vtkm::cont::ArrayHandle<T> result;
    switch (this->InputDimensionality)
    {
      case 1:
        this->MapPointField<1>(field, result);
        break;
      case 2:
        this->MapPointField<2>(field, result);
        break;
      case 3:
        this->MapPointField<3>(field, result);
        break;
      default:
        break;
    }

    return result;
  }

  template <typename T, typename Storage>
  vtkm::cont::ArrayHandle<T> ProcessCellField(
    const vtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    vtkm::cont::ArrayHandle<T> result;
    switch (this->InputDimensionality)
    {
      case 1:
        this->MapCellField<1>(field, result);
        break;
      case 2:
        this->MapCellField<2>(field, result);
        break;
      case 3:
        this->MapCellField<3>(field, result);
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
