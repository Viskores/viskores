## SSIM filter

The Structural Similarity Index Measure (SSIM) is a useful tool for comparing
images. SSIM was designed to better capture visual differences of images than
simple difference or L-norm measures. Additioinally, the SSIM metric tends to be
less sensitive to minor differences in rendering and is thus less likely to be
tripped up by, for example, lines drawn a pixel off. The new SSIM filter
performs this comparison for images.

The Viskores regression testing framework is updated to use the SSIM filter.
Previously, Viskores used pixel-wise differences to measure the match of the
generated image to a reference base image. The tests now use the SSIM metric,
which should make it easier to choose thresholds that are above differences in
renderers but below differences in algorithm results.
