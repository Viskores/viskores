//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_geometry_refinement_ExtrusionRotational_h
#define viskores_filter_geometry_refinement_ExtrusionRotational_h

#include <viskores/Math.h>
#include <viskores/filter/geometry_refinement/ExtrusionAbstract.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Revolve a triangulated profile into wedge cells.
///
/// `ExtrusionRotational` creates a series of rotated copies of the input points around an
/// axis and connects adjacent triangle copies with wedge cells. By default, the
/// output is an explicit wedge cell set for broad downstream compatibility.
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT ExtrusionRotational
  : public viskores::filter::geometry_refinement::ExtrusionAbstract
{
public:
  VISKORES_CONT ExtrusionRotational();

  /// @brief Set the axis direction used for rotation.
  VISKORES_CONT void SetAxis(viskores::Vec3f_64 axis) { this->Axis = axis; }
  /// @copydoc SetAxis
  VISKORES_CONT viskores::Vec3f_64 GetAxis() const { return this->Axis; }

  /// @brief Set a point on the rotation axis.
  VISKORES_CONT void SetCenter(viskores::Vec3f_64 center) { this->Center = center; }
  /// @copydoc SetCenter
  VISKORES_CONT viskores::Vec3f_64 GetCenter() const { return this->Center; }

  /// @brief Set the total sweep angle in radians.
  ///
  /// Unless `SetCloseSweep` has been called explicitly, full 2*pi sweeps are
  /// closed and non-full sweeps are open.
  VISKORES_CONT void SetSweepAngle(viskores::FloatDefault radians);
  /// @copydoc SetSweepAngle
  VISKORES_CONT viskores::FloatDefault GetSweepAngle() const { return this->SweepAngle; }

  /// @brief Explicitly specify whether the last plane connects back to the first.
  VISKORES_CONT void SetCloseSweep(bool close);
  /// @copydoc SetCloseSweep
  VISKORES_CONT bool GetCloseSweep() const { return this->CloseSweep; }

  /// @brief Allow points on the rotation axis, which can produce degenerate wedges.
  ///
  /// This is enabled by default to support solid-of-revolution inputs whose
  /// profile touches the rotation axis. Disable it to reject axis-touching
  /// profiles instead of producing degenerate wedges along the axis.
  VISKORES_CONT void SetAllowAxisPoints(bool on) { this->AllowAxisPoints = on; }
  /// @copydoc SetAllowAxisPoints
  VISKORES_CONT bool GetAllowAxisPoints() const { return this->AllowAxisPoints; }

  /// @brief Set the absolute distance tolerance used to detect points on the rotation axis.
  VISKORES_CONT void SetAxisPointTolerance(viskores::Float64 tolerance)
  {
    this->AxisPointTolerance = tolerance;
  }
  /// @copydoc SetAxisPointTolerance
  VISKORES_CONT viskores::Float64 GetAxisPointTolerance() const { return this->AxisPointTolerance; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Vec3f_64 Axis = { 0.0, 0.0, 1.0 };
  viskores::Vec3f_64 Center = { 0.0, 0.0, 0.0 };
  viskores::FloatDefault SweepAngle = viskores::TwoPi<viskores::FloatDefault>();
  bool CloseSweep = false;
  bool CloseSweepExplicitlySet = false;
  bool AllowAxisPoints = true;
  viskores::Float64 AxisPointTolerance = 1.0e-6;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_ExtrusionRotational_h
