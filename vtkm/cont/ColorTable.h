//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ColorTable_h
#define vtk_m_cont_ColorTable_h

#include <vtkm/Range.h>
#include <vtkm/Types.h>

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ColorTableSamples.h>

#include <set>

namespace vtkm
{

namespace exec
{
//forward declare exec objects
class ColorTableBase;
}

namespace cont
{

template <typename T>
class VirtualObjectHandle;


namespace detail
{
struct ColorTableInternals;
}

enum struct ColorSpace
{
  RGB,
  HSV,
  HSV_WRAP,
  LAB,
  DIVERGING
};

/// \brief Color Table for coloring arbitrary fields
///
///
/// The vtkm::cont::ColorTable allows for color mapping in RGB or HSV space and
/// uses a piecewise hermite functions to allow opacity interpolation that can be
/// piecewise constant, piecewise linear, or somewhere in-between
/// (a modified piecewise hermite function that squishes the function
/// according to a sharpness parameter).
///
/// For colors interpolation is handled using a piecewise linear function.
///
/// For opacity we define a piecewise function mapping. This mapping allows the addition
/// of control points, and allows the user to control the function between
/// the control points. A piecewise hermite curve is used between control
/// points, based on the sharpness and midpoint parameters. A sharpness of
/// 0 yields a piecewise linear function and a sharpness of 1 yields a
/// piecewise constant function. The midpoint is the normalized distance
/// between control points at which the curve reaches the median Y value.
/// The midpoint and sharpness values specified when adding a node are used
/// to control the transition to the next node with the last node's values being
/// ignored.
///
/// When adding opacity nodes without an explicit midpoint and sharpness we
/// will default to to Midpoint = 0.5 (halfway between the control points) and
/// Sharpness = 0.0 (linear).
///
/// ColorTable also contains which ColorSpace should be used for interpolation
/// Currently the valid ColorSpaces are:
/// - RGB
/// - HSV
/// - HSV_WRAP
/// - LAB
/// - Diverging
///
/// In HSV_WRAP mode, it will take the shortest path
/// in Hue (going back through 0 if that is the shortest way around the hue
/// circle) whereas HSV will not go through 0 (in order the
/// match the current functionality of vtkLookupTable). In Lab mode,
/// it will take the shortest path in the Lab color space with respect to the
/// CIE Delta E 2000 color distance measure. Diverging is a special
/// mode where colors will pass through white when interpolating between two
/// saturated colors.
///
/// To map a field from a vtkm::cont::DataSet through the color and opacity transfer
/// functions and into a RGB or RGBA array you should use vtkm::filter::FieldToColor.
///
class VTKM_CONT_EXPORT ColorTable
{
  std::shared_ptr<detail::ColorTableInternals> Impl;

public:
  enum struct Preset
  {
    DEFAULT,
    COOL_TO_WARM,
    COOL_TO_WARM_EXTENDED,
    VIRIDIS,
    INFERNO,
    PLASMA,
    BLACK_BODY_RADIATION,
    X_RAY,
    GREEN,
    BLACK_BLUE_WHITE,
    BLUE_TO_ORANGE,
    GRAY_TO_RED,
    COLD_AND_HOT,
    BLUE_GREEN_ORANGE,
    YELLOW_GRAY_BLUE,
    RAINBOW_UNIFORM,
    JET,
    RAINBOW_DESATURATED
  };

  /// \brief Construct a color table from a preset
  ///
  /// Constructs a color table from a given preset, which might include a NaN color.
  /// The alpha table will have 2 entries of alpha = 1.0 with linear interpolation
  ///
  /// Note: these are a select set of the presets you can get by providing a string identifier.
  ///
  ColorTable(vtkm::cont::ColorTable::Preset preset = vtkm::cont::ColorTable::Preset::DEFAULT);

  /// \brief Construct a color table from a preset color table
  ///
  /// Constructs a color table from a given preset, which might include a NaN color.
  /// The alpha table will have 2 entries of alpha = 1.0 with linear interpolation
  ///
  /// Note: Names are case insensitive
  /// Currently supports the following color tables:
  ///
  /// "Default"
  /// "Cool to Warm"
  /// "Cool to Warm Extended"
  /// "Viridis"
  /// "Inferno"
  /// "Plasma"
  /// "Black-Body Radiation"
  /// "X Ray"
  /// "Green"
  /// "Black - Blue - White"
  /// "Blue to Orange"
  /// "Gray to Red"
  /// "Cold and Hot"
  /// "Blue - Green - Orange"
  /// "Yellow - Gray - Blue"
  /// "Rainbow Uniform"
  /// "Jet"
  /// "Rainbow Desaturated"
  ///
  explicit ColorTable(const std::string& name);

  /// Construct a color table with a zero positions, and an invalid range
  ///
  /// Note: The color table will have 0 entries
  /// Note: The alpha table will have 0 entries
  explicit ColorTable(ColorSpace space);

  /// Construct a color table with a 2 positions
  ///
  /// Note: The color table will have 2 entries of rgb = {1.0,1.0,1.0}
  /// Note: The alpha table will have 2 entries of alpha = 1.0 with linear
  ///       interpolation
  ColorTable(const vtkm::Range& range, ColorSpace space = ColorSpace::LAB);

  /// Construct a color table with 2 positions
  //
  /// Note: The alpha table will have 2 entries of alpha = 1.0 with linear
  ///       interpolation
  ColorTable(const vtkm::Range& range,
             const vtkm::Vec<float, 3>& rgb1,
             const vtkm::Vec<float, 3>& rgb2,
             ColorSpace space = ColorSpace::LAB);

  /// Construct color and alpha and table with 2 positions
  ///
  /// Note: The alpha table will use linear interpolation
  ColorTable(const vtkm::Range& range,
             const vtkm::Vec<float, 4>& rgba1,
             const vtkm::Vec<float, 4>& rgba2,
             ColorSpace space = ColorSpace::LAB);

  /// Construct a color table with a list of colors and alphas. For this version you must also
  /// specify a name.
  ///
  /// This constructor is mostly used for presets.
  ColorTable(const std::string& name,
             vtkm::cont::ColorSpace colorSpace,
             const vtkm::Vec<double, 3>& nanColor,
             const std::vector<double>& rgbPoints,
             const std::vector<double>& alphaPoints = { 0.0, 1.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0 });


  ~ColorTable();

  ColorTable& operator=(const ColorTable&) = default;
  ColorTable(const ColorTable&) = default;

  const std::string& GetName() const;
  void SetName(const std::string& name);

  bool LoadPreset(vtkm::cont::ColorTable::Preset preset);

  /// Returns the name of all preset color tables
  ///
  /// This list will include all presets defined in vtkm::cont::ColorTable::Preset and could
  /// include extras as well.
  ///
  static std::set<std::string> GetPresets();

  /// Load a preset color table
  ///
  /// Removes all existing all values in both color and alpha tables,
  /// and will reset the NaN Color if the color table has that information.
  /// Will not modify clamping, below, and above range state.
  ///
  /// Note: Names are case insensitive
  ///
  /// Currently supports the following color tables:
  /// "Default"
  /// "Cool to Warm"
  /// "Cool to Warm Extended"
  /// "Viridis"
  /// "Inferno"
  /// "Plasma"
  /// "Black-Body Radiation"
  /// "X Ray"
  /// "Green"
  /// "Black - Blue - White"
  /// "Blue to Orange"
  /// "Gray to Red"
  /// "Cold and Hot"
  /// "Blue - Green - Orange"
  /// "Yellow - Gray - Blue"
  /// "Rainbow Uniform"
  /// "Jet"
  /// "Rainbow Desaturated"
  ///
  bool LoadPreset(const std::string& name);

  /// Make a deep copy of the current color table
  ///
  /// The ColorTable is implemented so that all stack based copies are 'shallow'
  /// copies. This means that they all alter the same internal instance. But
  /// sometimes you need to make an actual fully independent copy.
  ColorTable MakeDeepCopy();

  ///
  ColorSpace GetColorSpace() const;
  void SetColorSpace(ColorSpace space);

  /// If clamping is disabled values that lay out side
  /// the color table range are colored based on Below
  /// and Above settings.
  ///
  /// By default clamping is enabled
  void SetClampingOn() { this->SetClamping(true); }
  void SetClampingOff() { this->SetClamping(false); }
  void SetClamping(bool state);
  bool GetClamping() const;

  /// Color to use when clamping is disabled for any value
  /// that is below the given range
  ///
  /// Default value is {0,0,0}
  void SetBelowRangeColor(const vtkm::Vec<float, 3>& c);
  const vtkm::Vec<float, 3>& GetBelowRangeColor() const;

  /// Color to use when clamping is disabled for any value
  /// that is above the given range
  ///
  /// Default value is {0,0,0}
  void SetAboveRangeColor(const vtkm::Vec<float, 3>& c);
  const vtkm::Vec<float, 3>& GetAboveRangeColor() const;

  ///
  void SetNaNColor(const vtkm::Vec<float, 3>& c);
  const vtkm::Vec<float, 3>& GetNaNColor() const;

  /// Remove all existing values in both color and alpha tables.
  /// Does not remove the clamping, below, and above range state or colors.
  void Clear();

  /// Remove only color table values
  void ClearColors();

  /// Remove only alpha table values
  void ClearAlpha();

  /// Reverse the rgb values inside the color table
  void ReverseColors();

  /// Reverse the alpha, mid, and sharp values inside the opacity table.
  ///
  /// Note: To keep the shape correct the mid and sharp values of the last
  /// node are not included in the reversal
  void ReverseAlpha();

  /// Returns min and max position of all function points
  const vtkm::Range& GetRange() const;

  /// Rescale the color and opacity transfer functions to match the
  /// input range.
  void RescaleToRange(const vtkm::Range& range);

  // Functions for Colors

  /// Adds a point to the color function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: rgb values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 osn error.
  vtkm::Int32 AddPoint(double x, const vtkm::Vec<float, 3>& rgb);

  /// Adds a point to the color function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: hsv values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 on error.
  vtkm::Int32 AddPointHSV(double x, const vtkm::Vec<float, 3>& hsv);

  /// Add a line segment to the color function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: rgb1, and rgb2 values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point x1 (0 based), or -1 on error.
  vtkm::Int32 AddSegment(double x1,
                         const vtkm::Vec<float, 3>& rgb1,
                         double x2,
                         const vtkm::Vec<float, 3>& rgb2);

  /// Add a line segment to the color function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: hsv1, and hsv2 values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  vtkm::Int32 AddSegmentHSV(double x1,
                            const vtkm::Vec<float, 3>& hsv1,
                            double x2,
                            const vtkm::Vec<float, 3>& hsv2);

  /// Get the location, and rgb information for an existing point in the opacity function.
  ///
  /// Note: components 1-3 are rgb and will have values between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  bool GetPoint(vtkm::Int32 index, vtkm::Vec<double, 4>&) const;

  /// Update the location, and rgb information for an existing point in the color function.
  /// If the location value for the index is modified the point is removed from
  /// the function and re-inserted in the proper sorted location.
  ///
  /// Note: components 1-3 are rgb and must have values between 0 and 1.0 (inclusive).
  /// Return the new index of the updated point (0 based), or -1 on error.
  vtkm::Int32 UpdatePoint(vtkm::Int32 index, const vtkm::Vec<double, 4>&);

  /// Remove the Color function point that exists at exactly x
  ///
  /// Return true if the point x exists and has been removed
  bool RemovePoint(double x);

  /// Remove the Color function point n
  ///
  /// Return true if n >= 0 && n < GetNumberOfPoints
  bool RemovePoint(vtkm::Int32 index);

  /// Returns the number of points in the color function
  vtkm::Int32 GetNumberOfPoints() const;

  // Functions for Opacity

  /// Adds a point to the opacity function. If the point already exists, it
  /// will be updated to the new value. Uses a midpoint of 0.5 (halfway between the control points)
  /// and sharpness of 0.0 (linear).
  ///
  /// Note: alpha needs to be a value between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 on error.
  vtkm::Int32 AddPointAlpha(double x, float alpha) { return AddPointAlpha(x, alpha, 0.5f, 0.0f); }

  /// Adds a point to the opacity function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  vtkm::Int32 AddPointAlpha(double x, float alpha, float midpoint, float sharpness);

  /// Add a line segment to the opacity function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function. Uses a midpoint of
  /// 0.5 (halfway between the control points) and sharpness of 0.0 (linear).
  ///
  /// Note: alpha values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  vtkm::Int32 AddSegmentAlpha(double x1, float alpha1, double x2, float alpha2)
  {
    vtkm::Vec<float, 2> mid_sharp(0.5f, 0.0f);
    return AddSegmentAlpha(x1, alpha1, x2, alpha2, mid_sharp, mid_sharp);
  }

  /// Add a line segment to the opacity function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  vtkm::Int32 AddSegmentAlpha(double x1,
                              float alpha1,
                              double x2,
                              float alpha2,
                              const vtkm::Vec<float, 2>& mid_sharp1,
                              const vtkm::Vec<float, 2>& mid_sharp2);

  /// Get the location, alpha, midpoint and sharpness information for an existing
  /// point in the opacity function.
  ///
  /// Note: alpha, midpoint, and sharpness values all will be between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  bool GetPointAlpha(vtkm::Int32 index, vtkm::Vec<double, 4>&) const;

  /// Update the location, alpha, midpoint and sharpness information for an existing
  /// point in the opacity function.
  /// If the location value for the index is modified the point is removed from
  /// the function and re-inserted in the proper sorted location
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the new index of the updated point (0 based), or -1 on error.
  vtkm::Int32 UpdatePointAlpha(vtkm::Int32 index, const vtkm::Vec<double, 4>&);

  /// Remove the Opacity function point that exists at exactly x
  ///
  /// Return true if the point x exists and has been removed
  bool RemovePointAlpha(double x);

  /// Remove the Opacity function point n
  ///
  /// Return true if n >= 0 && n < GetNumberOfPointsAlpha
  bool RemovePointAlpha(vtkm::Int32 index);

  /// Returns the number of points in the alpha function
  vtkm::Int32 GetNumberOfPointsAlpha() const;

  /// Fill the Color table from a double pointer
  ///
  /// The double pointer is required to have the layout out of [X1, R1,
  /// G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
  /// number of nodes.
  /// This will remove any existing color control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Note: This is provided as a interoperability method with VTK
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillColorTableFromDataPointer(vtkm::Int32 n, const double* ptr);

  /// Fill the Color table from a float pointer
  ///
  /// The double pointer is required to have the layout out of [X1, R1,
  /// G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
  /// number of nodes.
  /// This will remove any existing color control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Note: This is provided as a interoperability method with VTK
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillColorTableFromDataPointer(vtkm::Int32 n, const float* ptr);

  /// Fill the Opacity table from a double pointer
  ///
  /// The double pointer is required to have the layout out of [X1, A1, M1, S1, X2, A2, M2, S2,
  /// ..., Xn, An, Mn, Sn] where n is the number of nodes. The Xi values represent the value to
  /// map, the Ai values represent alpha (opacity) value, the Mi values represent midpoints, and
  /// the Si values represent sharpness. Use 0.5 for midpoint and 0.0 for sharpness to have linear
  /// interpolation of the alpha.
  ///
  /// This will remove any existing opacity control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillOpacityTableFromDataPointer(vtkm::Int32 n, const double* ptr);

  /// Fill the Opacity table from a float pointer
  ///
  /// The float pointer is required to have the layout out of [X1, A1, M1, S1, X2, A2, M2, S2,
  /// ..., Xn, An, Mn, Sn] where n is the number of nodes. The Xi values represent the value to
  /// map, the Ai values represent alpha (opacity) value, the Mi values represent midpoints, and
  /// the Si values represent sharpness. Use 0.5 for midpoint and 0.0 for sharpness to have linear
  /// interpolation of the alpha.
  ///
  /// This will remove any existing opacity control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillOpacityTableFromDataPointer(vtkm::Int32 n, const float* ptr);


  /// \brief Sample each value through an intermediate lookup/sample table to generate RGBA colors
  ///
  /// Each value in \c values is binned based on its value in relationship to the range
  /// of the color table and will use the color value at that bin from the \c samples.
  /// To generate the lookup table use \c Sample .
  ///
  /// Here is a simple example.
  /// \code{.cpp}
  ///
  /// vtkm::cont::ColorTableSamplesRGBA samples;
  /// vtkm::cont::ColorTable table("black-body radiation");
  /// table.Sample(256, samples);
  /// vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
  /// table.Map(input, samples, colors);
  ///
  /// \endcode
  template <typename T, typename S>
  inline bool Map(const vtkm::cont::ArrayHandle<T, S>& values,
                  const vtkm::cont::ColorTableSamplesRGBA& samples,
                  vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Sample each value through an intermediate lookup/sample table to generate RGB colors
  ///
  /// Each value in \c values is binned based on its value in relationship to the range
  /// of the color table and will use the color value at that bin from the \c samples.
  /// To generate the lookup table use \c Sample .
  ///
  /// Here is a simple example.
  /// \code{.cpp}
  ///
  /// vtkm::cont::ColorTableSamplesRGB samples;
  /// vtkm::cont::ColorTable table("black-body radiation");
  /// table.Sample(256, samples);
  /// vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
  /// table.Map(input, samples, colors);
  ///
  /// \endcode
  template <typename T, typename S>
  inline bool Map(const vtkm::cont::ArrayHandle<T, S>& values,
                  const vtkm::cont::ColorTableSamplesRGB& samples,
                  vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector with a sample table to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           const vtkm::cont::ColorTableSamplesRGBA& samples,
                           vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector with a sample table to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           const vtkm::cont::ColorTableSamplesRGB& samples,
                           vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector with a sample table to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           vtkm::IdComponent comp,
                           const vtkm::cont::ColorTableSamplesRGBA& samples,
                           vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector with a sample table to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           vtkm::IdComponent comp,
                           const vtkm::cont::ColorTableSamplesRGB& samples,
                           vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;


  /// \brief Interpolate each value through the color table to generate RGBA colors
  ///
  /// Each value in \c values will be sampled through the entire color table
  /// to determine a color.
  ///
  /// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
  template <typename T, typename S>
  inline bool Map(const vtkm::cont::ArrayHandle<T, S>& values,
                  vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Interpolate each value through the color table to generate RGB colors
  ///
  /// Each value in \c values will be sampled through the entire color table
  /// to determine a color.
  ///
  /// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
  template <typename T, typename S>
  inline bool Map(const vtkm::cont::ArrayHandle<T, S>& values,
                  vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;

  /// \brief Use magnitude of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapMagnitude(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;

  /// \brief Use a single component of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           vtkm::IdComponent comp,
                           vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  inline bool MapComponent(const vtkm::cont::ArrayHandle<vtkm::Vec<T, N>, S>& values,
                           vtkm::IdComponent comp,
                           vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& rgbOut) const;


  /// \brief generate RGB colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either vtkm::Float32 or vtkm::Float64 space.
  /// Will use vtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(vtkm::Int32 numSamples,
                     vtkm::cont::ColorTableSamplesRGBA& samples,
                     double tolerance = 0.002) const;

  /// \brief generate a sample lookup table using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either vtkm::Float32 or vtkm::Float64 space.
  /// Will use vtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(vtkm::Int32 numSamples,
                     vtkm::cont::ColorTableSamplesRGB& samples,
                     double tolerance = 0.002) const;

  /// \brief generate RGBA colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either vtkm::Float32 or vtkm::Float64 space.
  /// Will use vtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(vtkm::Int32 numSamples,
                     vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>& colors,
                     double tolerance = 0.002) const;

  /// \brief generate RGB colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either vtkm::Float32 or vtkm::Float64 space.
  /// Will use vtkm::Float32 space when the difference between the float and double
  /// values when the range is within float space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  inline bool Sample(vtkm::Int32 numSamples,
                     vtkm::cont::ArrayHandle<vtkm::Vec3ui_8>& colors,
                     double tolerance = 0.002) const;


  /// \brief returns a virtual object pointer of the exec color table
  ///
  /// This pointer is only valid as long as the ColorTable is unmodified
  inline const vtkm::exec::ColorTableBase* PrepareForExecution(
    vtkm::cont::DeviceAdapterId deviceId) const;

  /// \brief returns the modified count for the virtual object handle of the exec color table
  ///
  /// The modified count allows consumers of a shared color table to keep track
  /// if the color table has been modified since the last time they used it.
  vtkm::Id GetModifiedCount() const;

  struct TransferState
  {
    bool NeedsTransfer;
    vtkm::exec::ColorTableBase* Portal;
    const vtkm::cont::ArrayHandle<double>& ColorPosHandle;
    const vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>>& ColorRGBHandle;
    const vtkm::cont::ArrayHandle<double>& OpacityPosHandle;
    const vtkm::cont::ArrayHandle<float>& OpacityAlphaHandle;
    const vtkm::cont::ArrayHandle<vtkm::Vec<float, 2>>& OpacityMidSharpHandle;
  };

private:
  bool NeedToCreateExecutionColorTable() const;

  //takes ownership of the pointer passed in
  void UpdateExecutionColorTable(
    vtkm::cont::VirtualObjectHandle<vtkm::exec::ColorTableBase>*) const;

  ColorTable::TransferState GetExecutionDataForTransfer() const;

  vtkm::exec::ColorTableBase* GetControlRepresentation() const;

  vtkm::cont::VirtualObjectHandle<vtkm::exec::ColorTableBase> const* GetExecutionHandle() const;
};
}
} //namespace vtkm::cont
#endif //vtk_m_cont_ColorTable_h
