---
date: "2016-05-29T19:40:58+02:00"
title:  "How to log"
description: "How to log"
weight: 3

menu:
  main:
    Parent: "tutorials"
    Weight: 3
---
        
Logging is implemented via the global class instance `osmscout::log`.
It can be found in the header `osmscout/util/Logging.h`.

Log levels are implemented via the methods `Debug()`, `Info()`, `Warn()`
and `Error()`. All of these methods in the parameterless variant
return a instance of `Logger::Line`. You can switch log levels on and off by
passing a boolean value to the above methods. `Logger::Line` uses the shift left
operator (`<<`) to output values. Currently supported datatypes are all internal
 types, `std::string`, and `osmscout::StopClock`.
 
`Logger::Line` uses a `Destination` to actually output the given parameter.

## Example

A simple example:

```c++
log.Error() << "Error getting areas from optimized areas index!";
```

A more complex example from the `MapPainter` class.

```c++
    if (parameter.IsDebugPerformance()) {
      log.Info()
          << "Paths: "
          << data.ways.size() << "/" << waysSegments << "/" << waysDrawn << "/" << waysLabelDrawn << " (pcs) "
          << prepareWaysTimer << "/" << pathsTimer << "/" << pathLabelsTimer << " (sec)";

      log.Info()
          << "Areas: "
          << data.areas.size() << "/" << areasSegments << "/" << areasDrawn << " (pcs) "
          << prepareAreasTimer << "/" << areasTimer << "/" << areaLabelsTimer << " (sec)";

      log.Info()
          << "Nodes: "
          << data.nodes.size() <<"+" << data.poiNodes.size() << "/" << nodesDrawn << " (pcs) "
          << nodesTimer << "/" << poisTimer << " (sec)";

      log.Info()
          << "Labels: " << labels.size() << "/" << overlayLabels.size() << "/" << labelsDrawn << " (pcs) "
          << labelsTimer << " (sec)";
    }
```

## Existing logger implementations

The default logger implementation is a set to `ConsoleLogger`. `Console` logger
logs debug and information level logs to `std::cout` and warning and error
logs to `std::cerr`.

`ConsoleLogger` uses `StreamLogger` as base class, which offers the required
classes (`StreamDestination`) to redirect logs to a implementation of
`std::ostream`.

The current othe rexisting Logger implementation is `NoOpLogger` which 
internally uses a `NoOpDestination`, which does nothing.

## How to redirect logging information

To redirect logging you implement the abstract baseclass `Logger` or inherit from
one of the existing implementations (`StreamLogger` might be adaquate
base class for most purposes).

You then implement the method `Logger::Log(Level level)` by return a
`Logger::Line` instatiated with your custom implementation of
`Logger::Destination`.
