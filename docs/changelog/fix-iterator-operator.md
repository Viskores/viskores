# Fixed operator for IteratorFromArrayPortal

There was an error in `operator-=` for `IteratorFromArrayPortal` that went
by unnoticed. The operator is fixed and regression tests for the operators
has been added.
