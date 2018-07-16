//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_waveletgenerator_h
#define vtk_m_worklet_waveletgenerator_h

#include <vtkm/Types.h>

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>

#include <vtkm/exec/FunctorBase.h>

#include <string>
#include <utility>

namespace vtkm
{
namespace worklet
{

/**
 * @brief The WaveletGenerator class creates a dataset similar to VTK's
 * vtkRTAnalyticSource.
 *
 * This class generates a predictable structured dataset with a smooth yet
 * interesting set of scalars, which is useful for testing and benchmarking.
 *
 * The GenerateDataSet method can be used to create a complete structured
 * dataset, while GenerateField will generate the scalar point field only.
 *
 * The scalars are computed as:
 *
 * ```
 * MaxVal * Gauss + MagX * sin(FrqX*x) + MagY * sin(FrqY*y) + MagZ * cos(FrqZ*z)
 * ```
 *
 * The dataset properties are determined by:
 * - `Minimum/MaximumExtent`: The logical point extents of the dataset.
 * - `Spacing`: The distance between points of the dataset.
 * - `Center`: The center of the dataset.
 *
 * The scalar functions is control via:
 * - `Center`: The center of a Gaussian contribution to the scalars.
 * - `StandardDeviation`: The unscaled width of a Gaussian contribution.
 * - `MaximumValue`: Upper limit of the scalar range.
 * - `Frequency`: The Frq[XYZ] parameters of the periodic contributions.
 * - `Magnitude`: The Mag[XYZ] parameters of the periodic contributions.
 *
 * By default, the following parameters are used:
 * - `Extents`: { -10, -10, -10 } `-->` { 10, 10, 10 }
 * - `Spacing`: { 1, 1, 1 }
 * - `Center`: { 0, 0, 0 }
 * - `StandardDeviation`: 0.5
 * - `MaximumValue`: 255
 * - `Frequency`: { 60, 30, 40 }
 * - `Magnitude`: { 10, 18, 5 }
 */
class WaveletGenerator
{
  using Vec3F = vtkm::Vec<vtkm::FloatDefault, 3>;

  Vec3F Center;
  Vec3F Spacing;
  Vec3F Frequency;
  Vec3F Magnitude;
  vtkm::Id3 MinimumExtent;
  vtkm::Id3 MaximumExtent;
  FloatDefault MaximumValue;
  FloatDefault StandardDeviation;

public:
  template <typename Device>
  struct Worker : public vtkm::exec::FunctorBase
  {
    using OutputHandleType = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
    using OutputPortalType =
      decltype(std::declval<OutputHandleType>().PrepareForOutput(0, Device()));

    Vec3F Center;
    Vec3F Spacing;
    Vec3F Frequency;
    Vec3F Magnitude;
    Vec3F MinimumPoint;
    Vec3F Scale;
    vtkm::Id3 Offset;
    vtkm::Id3 Dims;
    vtkm::FloatDefault MaximumValue;
    vtkm::FloatDefault Temp2;
    OutputHandleType Output;
    OutputPortalType Portal;

    VTKM_EXEC_CONT
    Worker(const Vec3F& center,
           const Vec3F& spacing,
           const Vec3F& frequency,
           const Vec3F& magnitude,
           const Vec3F& minimumPoint,
           const Vec3F& scale,
           const vtkm::Id3& offset,
           const vtkm::Id3& dims,
           vtkm::FloatDefault maximumValue,
           vtkm::FloatDefault temp2)
      : Center(center)
      , Spacing(spacing)
      , Frequency(frequency)
      , Magnitude(magnitude)
      , MinimumPoint(minimumPoint)
      , Scale(scale)
      , Offset(offset)
      , Dims(dims)
      , MaximumValue(maximumValue)
      , Temp2(temp2)
    {
      const vtkm::Id nVals = dims[0] * dims[1] * dims[2];
      this->Portal = this->Output.PrepareForOutput(nVals, Device());
    }

    VTKM_EXEC
    void operator()(const vtkm::Id3& ijk) const
    {
      // map ijk to the point location, accounting for spacing:
      const Vec3F loc = Vec3F(ijk + this->Offset) * this->Spacing;

      // Compute the distance from the center of the gaussian:
      const Vec3F scaledLoc = (this->Center - loc) * this->Scale;
      vtkm::FloatDefault gaussSum = vtkm::Dot(scaledLoc, scaledLoc);

      const Vec3F periodicContribs{
        this->Magnitude[0] * vtkm::Sin(this->Frequency[0] * scaledLoc[0]),
        this->Magnitude[1] * vtkm::Sin(this->Frequency[1] * scaledLoc[1]),
        this->Magnitude[2] * vtkm::Cos(this->Frequency[2] * scaledLoc[2]),
      };

      // The vtkRTAnalyticSource documentation says the periodic contributions
      // should be multiplied in, but the implementation adds them. We'll do as
      // they do, not as they say.
      const vtkm::FloatDefault scalar = this->MaximumValue * vtkm::Exp(-gaussSum * this->Temp2) +
        periodicContribs[0] + periodicContribs[1] + periodicContribs[2];

      // Compute output location
      // (see ConnectivityStructuredInternals<3>::LogicalToFlatPointIndex)
      const vtkm::Id scalarIdx = ijk[0] + this->Dims[0] * (ijk[1] + this->Dims[1] * ijk[2]);
      this->Portal.Set(scalarIdx, scalar);
    }
  };

  VTKM_CONT
  WaveletGenerator()
    : Center{ 0. }
    , Spacing{ 1. }
    , Frequency{ 60., 30., 40. }
    , Magnitude{ 10., 18., 5. }
    , MinimumExtent{ -10 }
    , MaximumExtent{ 10 }
    , MaximumValue{ 255. }
    , StandardDeviation{ 0.5 }
  {
  }

  VTKM_CONT void SetCenter(const vtkm::Vec<FloatDefault, 3>& center) { this->Center = center; }

  VTKM_CONT void SetSpacing(const vtkm::Vec<FloatDefault, 3>& spacing) { this->Spacing = spacing; }

  VTKM_CONT void SetFrequency(const vtkm::Vec<FloatDefault, 3>& frequency)
  {
    this->Frequency = frequency;
  }

  VTKM_CONT void SetMagnitude(const vtkm::Vec<FloatDefault, 3>& magnitude)
  {
    this->Magnitude = magnitude;
  }

  VTKM_CONT void SetMinimumExtent(const vtkm::Id3& minExtent) { this->MinimumExtent = minExtent; }

  VTKM_CONT void SetMaximumExtent(const vtkm::Id3& maxExtent) { this->MaximumExtent = maxExtent; }

  VTKM_CONT void SetExtent(const vtkm::Id3& minExtent, const vtkm::Id3& maxExtent)
  {
    this->MinimumExtent = minExtent;
    this->MaximumExtent = maxExtent;
  }

  VTKM_CONT void SetMaximumValue(const vtkm::FloatDefault& maxVal) { this->MaximumValue = maxVal; }

  VTKM_CONT void SetStandardDeviation(const vtkm::FloatDefault& stdev)
  {
    this->StandardDeviation = stdev;
  }

  template <typename Device>
  VTKM_CONT vtkm::cont::DataSet GenerateDataSet(Device = Device())
  {
    // Create points:
    const vtkm::Id3 dims{ this->MaximumExtent - this->MinimumExtent };
    const Vec3F origin{ this->MinimumExtent };
    vtkm::cont::CoordinateSystem coords{ "coords", dims, origin, this->Spacing };

    // And cells:
    vtkm::cont::CellSetStructured<3> cellSet{ "cells" };
    cellSet.SetPointDimensions(dims);

    // Scalars, too
    vtkm::cont::Field field = this->GenerateField<Device>("scalars");

    // Compile the dataset:
    vtkm::cont::DataSet dataSet;
    dataSet.AddCoordinateSystem(coords);
    dataSet.AddCellSet(cellSet);
    dataSet.AddField(field);

    return dataSet;
  }

  template <typename Device>
  VTKM_CONT vtkm::cont::Field GenerateField(const std::string& name, Device = Device())
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<Device>;

    const vtkm::Id3 dims{ this->MaximumExtent - this->MinimumExtent };
    Vec3F minPt = Vec3F(this->MinimumExtent) * this->Spacing;
    vtkm::FloatDefault temp2 = 1.f / (2.f * this->StandardDeviation * this->StandardDeviation);
    Vec3F scale{ ComputeScaleFactor(this->MinimumExtent[0], this->MaximumExtent[0]),
                 ComputeScaleFactor(this->MinimumExtent[1], this->MaximumExtent[1]),
                 ComputeScaleFactor(this->MinimumExtent[2], this->MaximumExtent[2]) };

    Worker<Device> worker{ this->Center,
                           this->Spacing,
                           this->Frequency,
                           this->Magnitude,
                           minPt,
                           scale,
                           this->MinimumExtent,
                           dims,
                           this->MaximumValue,
                           temp2 };

    Algo::Schedule(worker, dims);

    return vtkm::cont::Field(name, vtkm::cont::Field::Association::POINTS, worker.Output);
  }

private:
  static vtkm::FloatDefault ComputeScaleFactor(vtkm::Id min, vtkm::Id max)
  {
    return (min < max) ? (1.f / static_cast<vtkm::FloatDefault>(max - min))
                       : static_cast<vtkm::FloatDefault>(1.);
  }
};
}
} // end namespace vtkm::worklet

#endif // vtk_m_worklet_waveletgenerator_h
