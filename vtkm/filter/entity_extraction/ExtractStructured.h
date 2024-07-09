//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_entity_extraction_ExtractStructured_h
#define vtk_m_filter_entity_extraction_ExtractStructured_h

#include <vtkm/RangeId3.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief Select a piece (e.g., volume of interest) and/or subsample structured points dataset
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
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExtractStructured : public vtkm::filter::Filter
{
public:
  /// @brief Specifies what volume of interest (VOI) should be extracted by the filter.
  ///
  /// The VOI is specified using the 3D indices of the structured mesh. Meshes with fewer
  /// than 3 dimensions will ignore the extra dimensions in the VOI. The VOI is inclusive
  /// on the minium index and exclusive on the maximum index.
  ///
  /// By default the VOI is the entire input.
  VTKM_CONT
  vtkm::RangeId3 GetVOI() const { return this->VOI; }

  /// @copydoc GetVOI
  VTKM_CONT
  void SetVOI(vtkm::Id i0, vtkm::Id i1, vtkm::Id j0, vtkm::Id j1, vtkm::Id k0, vtkm::Id k1)
  {
    this->VOI = vtkm::RangeId3(i0, i1, j0, j1, k0, k1);
  }
  /// @copydoc GetVOI
  VTKM_CONT
  void SetVOI(vtkm::Id extents[6]) { this->VOI = vtkm::RangeId3(extents); }
  /// @copydoc GetVOI
  VTKM_CONT
  void SetVOI(vtkm::Id3 minPoint, vtkm::Id3 maxPoint)
  {
    this->VOI = vtkm::RangeId3(minPoint, maxPoint);
  }
  /// @copydoc GetVOI
  VTKM_CONT
  void SetVOI(const vtkm::RangeId3& voi) { this->VOI = voi; }

  /// @brief Specifies the sample rate of the VOI.
  ///
  /// The input data can be subsampled by selecting every n-th value.
  /// The sampling can be different in each dimension.
  /// The default sampling rate is (1,1,1), meaning that no subsampling will occur.
  VTKM_CONT
  vtkm::Id3 GetSampleRate() const { return this->SampleRate; }

  /// @copydoc GetSampleRate
  VTKM_CONT
  void SetSampleRate(vtkm::Id i, vtkm::Id j, vtkm::Id k) { this->SampleRate = vtkm::Id3(i, j, k); }

  /// @copydoc GetSampleRate
  VTKM_CONT
  void SetSampleRate(vtkm::Id3 sampleRate) { this->SampleRate = sampleRate; }

  /// @brief Specifies if the outer boundary should always be included.
  ///
  /// When a subsample rate is specified, it is possible that some of the
  /// boundary samples will not be included in the sampling. If this is the
  /// case and `IncludeBoundary` is set to true, then an extra sample is
  /// set in the output and the values on the boundary are included. For example,
  /// say the input has resolution (5, 5, 1) (and the VOI matches), and the sample
  /// rate is set to (3, 3, 1). If `IncludeBoundary` is false, then the output will
  /// have the 4 points that correspond to the 3D indices (0, 0, 0), (3, 0, 0),
  /// (0, 3, 0), and (3, 3, 0) of the input. This misses the outer boundary at
  /// index 4 in the x and y directions. If `IncludeBoundary is set to true, then
  /// the output will have the 9 points that correspond to the 3D indices (0, 0, 0),
  /// (3, 0, 0), (4, 0, 0), (0, 3, 0), (3, 3, 0), (4, 3, 0), (0, 4, 0), (3, 4, 0),
  /// and (4, 4, 0) to capture this outer boundary.
  VTKM_CONT
  bool GetIncludeBoundary() const { return this->IncludeBoundary; }
  /// @copydoc GetIncludeBoundary
  VTKM_CONT
  void SetIncludeBoundary(bool value) { this->IncludeBoundary = value; }

  // Set if VOI is specified in global (rather than in local) point indices
  // (NOTE: Deprecated this method since this does not seem to work as
  // expected and there are no tests for it. Furthermore, neither VTK-m nor
  // VTK-h/Ascent seem to use this method. If your are using this method
  // somewhere else and think it should remain, please open a merge request to
  // "de-deprecate" it and add a test and documentation of the expected
  // behavior.)
  VTKM_DEPRECATED(2.1)
  VTKM_CONT
  void SetIncludeOffset(bool value) { this->IncludeOffset = value; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::RangeId3 VOI = vtkm::RangeId3(0, -1, 0, -1, 0, -1);
  vtkm::Id3 SampleRate = { 1, 1, 1 };
  bool IncludeBoundary = false;
  bool IncludeOffset = false;
};

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_ExtractStructured_h
