# Remove deprecated features from VTK-m

With the major revision 2.0 of VTK-m, many items previously marked as
deprecated were removed. If updating to a new version of VTK-m, it is
recommended to first update to VTK-m 1.9, which will include the deprecated
features but provide warnings (with the right compiler) that will point to
the replacement code. Once the deprecations have been fixed, updating to
2.0 should be smoother.
