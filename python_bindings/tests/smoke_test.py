import numpy as np

import viskores


def main():
    dims = (4, 3, 2)
    ds = viskores.create_uniform_dataset(dims, origin=(0.0, 0.0, 0.0), spacing=(1.0, 2.0, 3.0))

    point_count = np.prod(dims)
    scalar = np.linspace(0.0, 1.0, point_count, dtype=np.float64)
    vector = np.stack([scalar, scalar * 2.0, scalar * 3.0], axis=1)

    ds.add_point_field("temperature", scalar)
    ds.add_point_field("velocity", vector)

    avg = viskores.cell_average(ds, "temperature", output_field_name="temperature_cells")
    grad = viskores.gradient(ds, "temperature", output_field_name="temperature_grad")
    mag = viskores.vector_magnitude(ds, "velocity", output_field_name="speed")
    iso = viskores.contour(ds, "temperature", [0.5])

    assert ds.number_of_points == point_count
    assert avg.get_field("temperature_cells").ndim == 1
    assert grad.get_field("temperature_grad").ndim == 2
    assert grad.get_field("temperature_grad").shape[1] >= 3
    assert mag.get_field("speed").ndim == 1
    assert iso.number_of_cells > 0


if __name__ == "__main__":
    main()
