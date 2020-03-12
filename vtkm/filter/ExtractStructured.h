//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExtractStructured_h
#define vtk_m_filter_ExtractStructured_h

#include <vtkm/filter/vtkm_filter_export.h>

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/ExtractStructured.h>

namespace vtkm
{
namespace filter
{
/// \brief Select piece (e.g., volume of interest) and/or subsample structured points dataset
///
/// Select or subsample a portion of an input structured dataset. The selected
/// portion of interested is referred to as the Volume Of Interest, or VOI.
/// The output of this filter is a structured dataset. The filter treats input
/// data of any topological dimension (i.e., point, line, plane, or volume) and
/// can generate output data of any topological dimension.
///
/// To use this filter set the VOI ivar which are i-j-k min/max indices that
/// specify a rectangular region in the data. (Note that these are 0-offset.)
/// You can also specify a sampling rate to subsample the data.
///
/// Typical applications of this filter are to extract a slice from a volume
/// for image processing, subsampling large volumes to reduce data size, or
/// extracting regions of a volume with interesting data.
///
class VTKM_ALWAYS_EXPORT ExtractStructured : public vtkm::filter::FilterDataSet<ExtractStructured>
{
public:
  VTKM_FILTER_EXPORT
  ExtractStructured();

  // Set the bounding box for the volume of interest
  VTKM_CONT
  vtkm::RangeId3 GetVOI() const { return this->VOI; }

  VTKM_CONT
  void SetVOI(vtkm::Id i0, vtkm::Id i1, vtkm::Id j0, vtkm::Id j1, vtkm::Id k0, vtkm::Id k1)
  {
    this->VOI = vtkm::RangeId3(i0, i1, j0, j1, k0, k1);
  }
  VTKM_CONT
  void SetVOI(vtkm::Id extents[6]) { this->VOI = vtkm::RangeId3(extents); }
  VTKM_CONT
  void SetVOI(vtkm::Id3 minPoint, vtkm::Id3 maxPoint)
  {
    this->VOI = vtkm::RangeId3(minPoint, maxPoint);
  }
  VTKM_CONT
  void SetVOI(const vtkm::RangeId3& voi) { this->VOI = voi; }

  /// Get the Sampling rate
  VTKM_CONT
  vtkm::Id3 GetSampleRate() const { return this->SampleRate; }

  /// Set the Sampling rate
  VTKM_CONT
  void SetSampleRate(vtkm::Id i, vtkm::Id j, vtkm::Id k) { this->SampleRate = vtkm::Id3(i, j, k); }

  /// Set the Sampling rate
  VTKM_CONT
  void SetSampleRate(vtkm::Id3 sampleRate) { this->SampleRate = sampleRate; }

  /// Get if we should include the outer boundary on a subsample
  VTKM_CONT
  bool GetIncludeBoundary() { return this->IncludeBoundary; }
  /// Set if we should include the outer boundary on a subsample
  VTKM_CONT
  void SetIncludeBoundary(bool value) { this->IncludeBoundary = value; }

  VTKM_CONT
  void SetIncludeOffset(bool value) { this->IncludeOffset = value; }

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    if (fieldMeta.IsPointField())
    {
      vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessPointField(input);

      result.AddField(fieldMeta.AsField(output));
      return true;
    }

    // cell data must be scattered to the cells created per input cell
    if (fieldMeta.IsCellField())
    {
      vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessCellField(input);

      result.AddField(fieldMeta.AsField(output));
      return true;
    }

    return false;
  }

private:
  vtkm::RangeId3 VOI;
  vtkm::Id3 SampleRate = { 1, 1, 1 };
  bool IncludeBoundary;
  bool IncludeOffset;
  vtkm::worklet::ExtractStructured Worklet;
};

#ifndef vtkm_filter_ExtractStructured_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(ExtractStructured);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/ExtractStructured.hxx>

#endif // vtk_m_filter_ExtractStructured_h
