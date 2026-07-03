## Make ANARI mapper appearance updates deterministic

ANARI point, triangle, and volume mappers now sample their supplied Viskores
color table instead of using hard-coded transfer functions. Their default
value range follows the primary field, and explicit constant ranges no longer
produce non-finite sampler transforms.

Mapper names, color tables, value ranges, volume opacity scales, field-mapping
options, actors, and raw ANARI color arrays now update objects that have already
been materialized as well as objects created later. Raw color arrays have
documented transfer or retained ownership according to `releaseArrays`.
