//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/source/Wavelet.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace
{
inline vtkm::FloatDefault computeScaleFactor(vtkm::Id min, vtkm::Id max)
{
  return (min < max) ? (1.f / static_cast<vtkm::FloatDefault>(max - min))
                     : static_cast<vtkm::FloatDefault>(1.);
}
}
namespace vtkm
{
namespace source
{
namespace wavelet
{

struct WaveletField : public vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn, FieldOut v);
  using ExecutionSignature = void(ThreadIndices, _2);
  using InputDomain = _1;

  using Vec3F = vtkm::Vec3f;

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

  WaveletField(const Vec3F& center,
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
  }

  template <typename ThreadIndexType>
  VTKM_EXEC void operator()(const ThreadIndexType& threadIndex, vtkm::FloatDefault& scalar) const
  {
    const vtkm::Id3 ijk = threadIndex.GetInputIndex3D();

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
    scalar =
      this->MaximumValue * vtkm::Exp(-gaussSum * this->Temp2) + vtkm::ReduceSum(periodicContribs);
  }
};
} // namespace wavelet

Wavelet::Wavelet(vtkm::Id3 minExtent, vtkm::Id3 maxExtent)
  : Center{ minExtent - ((minExtent - maxExtent) / 2) }
  , Origin{ minExtent }
  , Spacing{ 1. }
  , Frequency{ 60., 30., 40. }
  , Magnitude{ 10., 18., 5. }
  , MinimumExtent{ minExtent }
  , MaximumExtent{ maxExtent }
  , MaximumValue{ 255. }
  , StandardDeviation{ 0.5 }
{
}

template <vtkm::IdComponent Dim>
vtkm::cont::DataSet Wavelet::GenerateDataSet(vtkm::cont::CoordinateSystem coords) const
{
  // And cells:
  vtkm::Vec<vtkm::Id, Dim> dims;
  for (unsigned int d = 0; d < Dim; d++)
  {
    dims[d] = this->MaximumExtent[d] - this->MinimumExtent[d] + 1;
  }
  vtkm::cont::CellSetStructured<Dim> cellSet;
  cellSet.SetPointDimensions(dims);

  // Compile the dataset:
  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(coords);
  dataSet.SetCellSet(cellSet);

  // Scalars, too
  vtkm::cont::Field field = this->GeneratePointField(cellSet, "RTData");
  dataSet.AddField(field);

  return dataSet;
}

vtkm::cont::DataSet Wavelet::Execute() const
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  // Create points:
  const vtkm::Id3 dims{ this->MaximumExtent - this->MinimumExtent + vtkm::Id3{ 1 } };
  vtkm::cont::CoordinateSystem coords{ "coordinates", dims, this->Origin, this->Spacing };

  // Compile the dataset:
  if (this->MaximumExtent[2] - this->MinimumExtent[2] < vtkm::Epsilon<vtkm::FloatDefault>())
  {
    return this->GenerateDataSet<2>(coords);
  }
  else
  {
    return this->GenerateDataSet<3>(coords);
  }
}

template <vtkm::IdComponent Dim>
vtkm::cont::Field Wavelet::GeneratePointField(const vtkm::cont::CellSetStructured<Dim>& cellset,
                                              const std::string& name) const
{
  const vtkm::Id3 dims{ this->MaximumExtent - this->MinimumExtent + vtkm::Id3{ 1 } };
  vtkm::Vec3f minPt = vtkm::Vec3f(this->MinimumExtent) * this->Spacing;
  vtkm::FloatDefault temp2 = 1.f / (2.f * this->StandardDeviation * this->StandardDeviation);
  vtkm::Vec3f scale{ computeScaleFactor(this->MinimumExtent[0], this->MaximumExtent[0]),
                     computeScaleFactor(this->MinimumExtent[1], this->MaximumExtent[1]),
                     computeScaleFactor(this->MinimumExtent[2], this->MaximumExtent[2]) };

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> output;
  wavelet::WaveletField worklet{ this->Center,
                                 this->Spacing,
                                 this->Frequency,
                                 this->Magnitude,
                                 minPt,
                                 scale,
                                 this->MinimumExtent,
                                 dims,
                                 this->MaximumValue,
                                 temp2 };
  this->Invoke(worklet, cellset, output);
  return vtkm::cont::make_FieldPoint(name, output);
}

} // namespace source
} // namespace vtkm
