//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#ifndef viskores_filter_image_processing_ImageMedian_h
#define viskores_filter_image_processing_ImageMedian_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/image_processing/viskores_filter_image_processing_export.h>

/// \brief Median algorithm for general image blur
///
/// The ImageMedian filter finds the median value for each pixel in an image.
/// Currently the algorithm has the following restrictions.
///   - Only supports a neighborhood of 5x5x1 or 3x3x1
///
/// This means that volumes are basically treated as an image stack
/// along the z axis
///
/// Default output field name is 'median'
namespace viskores
{
namespace filter
{
namespace image_processing
{
class VISKORES_FILTER_IMAGE_PROCESSING_EXPORT ImageMedian : public viskores::filter::Filter
{
public:
  VISKORES_CONT ImageMedian() { this->SetOutputFieldName("median"); }

  /// @brief Perform a median of the 3x3 neighborhood around each pixel.
  ///
  /// This establishes the neighborhood to use the next time `Execute()` is called.
  /// This is the default behavior.
  VISKORES_CONT void Perform3x3() { this->Neighborhood = 1; };
  /// @brief Perform a median of the 5x5 neighborhood around each pixel.
  ///
  /// This establishes the neighborhood to use the next time `Execute()` is called.
  VISKORES_CONT void Perform5x5() { this->Neighborhood = 2; };

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  int Neighborhood = 1;
};
} // namespace image_processing
} // namespace filter
} // namespace viskores

#endif //viskores_filter_image_processing_ImageMedian_h
