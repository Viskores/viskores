##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np

import viskores


dims = (16, 16, 16)
dataset = viskores.create_uniform_dataset(dims, spacing=(0.5, 0.5, 0.5))

point_count = np.prod(dims)
temperature = np.linspace(0.0, 1.0, point_count, dtype=np.float64)
velocity = np.stack([temperature, temperature**2, 1.0 - temperature], axis=1)

dataset.add_point_field("temperature", temperature)
dataset.add_point_field("velocity", velocity)

gradient = viskores.gradient(dataset, "temperature", output_field_name="temperature_gradient")
magnitude = viskores.vector_magnitude(dataset, "velocity", output_field_name="speed")
contour = viskores.contour(dataset, "temperature", [0.25, 0.5, 0.75])

print("gradient field shape:", gradient.get_field("temperature_gradient").shape)
print("speed field shape:", magnitude.get_field("speed").shape)
print("contour cells:", contour.number_of_cells)
