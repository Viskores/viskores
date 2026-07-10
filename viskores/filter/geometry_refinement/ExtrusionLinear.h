//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_geometry_refinement_ExtrusionLinear_h
#define viskores_filter_geometry_refinement_ExtrusionLinear_h

#include <viskores/filter/geometry_refinement/ExtrusionAbstract.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Extrude a triangulated profile along a direction into wedge cells.
///
/// `ExtrusionLinear` creates a sequence of translated copies of the input points
/// along a direction and connects adjacent triangle copies with wedge cells.
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT ExtrusionLinear
  : public viskores::filter::geometry_refinement::ExtrusionAbstract
{
public:
  VISKORES_CONT ExtrusionLinear() { this->SetNumberOfPlanes(2); }

  /// @brief Set the extrusion direction.
  ///
  /// The direction vector is normalized before use. Use `SetDistance` to set
  /// the spacing between adjacent planes.
  VISKORES_CONT void SetDirection(viskores::Vec3f direction) { this->Direction = direction; }
  /// @copydoc SetDirection
  VISKORES_CONT viskores::Vec3f GetDirection() const { return this->Direction; }

  /// @brief Set the extrusion distance along the normalized direction.
  VISKORES_CONT void SetDistance(viskores::FloatDefault distance) { this->Distance = distance; }
  /// @copydoc SetDistance
  VISKORES_CONT viskores::FloatDefault GetDistance() const { return this->Distance; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Vec3f Direction = { 0, 0, 1 };
  viskores::FloatDefault Distance = 1;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_ExtrusionLinear_h
