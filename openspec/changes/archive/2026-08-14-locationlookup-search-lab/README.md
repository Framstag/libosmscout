# locationlookup-search-lab

Extend the `LocationLookup` demo into a search-result inspection tool: merged structured + fulltext search with source selection (`--structured-only`, `--fulltext-only`), rank-sorted output with explicit `typeRank × distanceRank × matchRank` components, tunable weights (`--weights`), and search-center flags (`--lat`, `--lon`). Fulltext support is MARISA-gated like `LookupText`. No core library or JavaScout changes.

See [proposal.md](proposal.md), [design.md](design.md), [tasks.md](tasks.md), and specs under `specs/`.
