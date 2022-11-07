# Correct particle density filter output field

The field being created by `ParticleDensityNearestGridPoint` was supposed
to be associated with cells, but it was sized to the number of points.
Although the number of points will always be more than the number of cells
(so the array will be big enough), having inappropriately sized arrays can
cause further problems downstream.
