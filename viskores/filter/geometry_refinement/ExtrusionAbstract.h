//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_geometry_refinement_ExtrusionAbstract_h
#define viskores_filter_geometry_refinement_ExtrusionAbstract_h

#include <viskores/Math.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

#include <string>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Base class for extrusion filters.
///
/// `ExtrusionAbstract` provides common controls for filters that create a sequence of
/// extruded profile planes and connect adjacent planes into an output cell set. Current
/// subclasses support triangulated profiles and wedge-cell output.
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT ExtrusionAbstract : public viskores::filter::Filter
{
public:
  /// @brief Set the number of profile planes.
  VISKORES_CONT void SetNumberOfPlanes(viskores::Id planes) { this->NumberOfPlanes = planes; }
  /// @copydoc SetNumberOfPlanes
  VISKORES_CONT viskores::Id GetNumberOfPlanes() const { return this->NumberOfPlanes; }

  /// @brief Triangulate the input before extrusion.
  VISKORES_CONT void SetTriangulateInput(bool on) { this->TriangulateInput = on; }
  /// @copydoc SetTriangulateInput
  VISKORES_CONT bool GetTriangulateInput() const { return this->TriangulateInput; }

  /// @brief Use compact `viskores::cont::CellSetExtrude` topology for the output.
  ///
  /// The default is `false`, which returns explicit wedge cells. Compact output
  /// uses less memory but requires downstream consumers that support
  /// `viskores::cont::CellSetExtrude`.
  VISKORES_CONT void SetCompactOutput(bool on) { this->CompactOutput = on; }
  /// @copydoc SetCompactOutput
  VISKORES_CONT bool GetCompactOutput() const { return this->CompactOutput; }

protected:
  template <typename MakeCoordinateWorklet,
            typename MakeOrientationWorklet,
            typename ValidateCoordinates>
  VISKORES_CONT viskores::cont::DataSet ExecuteTriangleExtrusion(
    const viskores::cont::DataSet& input,
    const std::string& filterName,
    bool closeSweep,
    MakeCoordinateWorklet makeCoordinateWorklet,
    MakeOrientationWorklet makeOrientationWorklet,
    ValidateCoordinates validateCoordinates);

private:
  viskores::Id NumberOfPlanes = 16;
  bool TriangulateInput = false;
  bool CompactOutput = false;
};

} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_ExtrusionAbstract_h
