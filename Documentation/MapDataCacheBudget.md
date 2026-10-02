# Map data cache memory budget

A client that opens several map databases — a basemap plus the region databases a user travels
through — keeps map data of every one of them in memory. The size of those caches used to be fixed per
database and independent of what the user was looking at, so the resident memory grew with the number
of open databases while only a few of them supplied the visible map. The memory budget bounds the map
data caches of **all open databases together**.

## What is bounded

| Cache | Per | Bounded by |
|-------|-----|------------|
| Tile cache of a database's map service | database | accounted weight |
| Object caches of `nodes.dat`, `ways.dat`, `areas.dat`, `routes.dat` | database | accounted weight |
| Cache of the area-area index | database | accounted weight |

Not part of the budget: the caches of the location and text search indexes, the low-zoom optimization
data (which keeps no cache), and the raster tile caches of the Qt renderers (`TiledMapRenderer`),
which hold pixels rather than map data.

## Accounting unit

An entry count is not comparable between kinds of objects: a cached area carries the points of its
rings and a precomputed bounding box per segment, a cached node a single coordinate. The content of a
cache is therefore accounted in weight units, with one weight per kind:

| Kind | Weight |
|------|--------|
| Node | 1 |
| Way (including optimized ways) | 3 |
| Area (including optimized areas) | 30 |
| Route | 3 |
| Index entry (area-area index) | 1 |

One weight unit stands for 128 bytes. The weights and the byte figure are defined once, as the
constants of `osmscout::MapDataAccounting`, and the reported usage is that figure — an accounting of
the cached entries, not a measurement of the heap of the process.

Objects that several tiles reference are accounted by every one of those tiles, so the accounted size
of a tile cache overestimates the memory it holds. The overestimate is the safe direction: the cache
drops tiles earlier rather than later.

## Distribution over the databases

- A database is **relevant** when its geographic extent intersects the area of the view.
- The relevant databases share the budget. Every other database is reduced to the **floor**.
- The floor of every registered cache is reserved in the budget. A budget that cannot hold the floor
  of every cache lowers the effective floor instead of being exceeded. The sum of the shares of all
  databases is at or below the budget.
- A database's share is split between its tile cache (a quarter of the share) and its object caches.
  The object caches never grow beyond the sizes configured for the database: a budget larger than the
  configured caches does not enlarge them.
- The distribution is applied only after the set of relevant databases has been stable for the
  **settling period** (default 5 seconds), so panning along the border of two databases does not
  resize the caches on every frame. A database that becomes relevant again inside that period keeps
  its share.
- A database that stays out of view for longer than the **idle period** (default 10 minutes) has its
  content released: the tile cache drops its tiles and the object caches of the database are flushed.
  It reloads its data when it becomes relevant again.

The bounds are enforced when a view is reported, so a client that keeps rendering keeps the caches of
the regions the user has left bounded.

## Configuring a budget

The budget is **off by default in the library**: without one, every database is bounded by the cache
sizes configured for it, exactly as before. The shipped clients configure a default budget, because
they are the ones that run where the memory limit is reached:

| Client | Default | How an application changes it |
|--------|---------|-------------------------------|
| Qt (`libosmscout-client-qt`) | 64 MiB | `OSMScoutQtBuilder::WithDataCacheBudget(bytes)`; `0` drops the bound |
| Java (`libosmscout-client-java`) | 64 MiB | `OSMScoutClient::setNativeDataCacheBudget(bytes)`; a non-positive value restores the default |

The per-database cache size keeps its meaning next to the budget: it is the capacity of a single
database's tile cache (`OSMScoutClient::setNativeDataCacheSize`, `MapService::SetCacheSize`) and it
remains the only bound when no budget is set.

## Reading the usage

- C++: `osmscout::MapService::GetAccountedWeight()` for one database,
  `osmscout::MapDataBudget::GetUsage()` for all registered databases together.
- Qt: the budget of a client is `DBThread::GetDataCacheBudget()`.
- Java: `OSMScoutClient::getNativeDataCacheUsage()`.
