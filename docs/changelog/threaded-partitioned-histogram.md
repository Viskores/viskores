## Support threaded partitioned histograms

Partitioned `Histogram` execution now uses the filter scheduler when computing field ranges and
initializes its shared bin width before concurrently processing partitions. This allows
partitioned histograms to safely use the standard multithreaded filter settings.
