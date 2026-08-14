# locationlookup-merged-search Specification

## Purpose
The LocationLookup demo performs both structured location search and fulltext text-index search, merges them into one result list (deduplicated, limited), marks each result with its source, and reports how many fulltext hits were truncated by the limit. Source selection switches allow running either half alone.

## Requirements

### Requirement: Merged structured and fulltext search by default

Running `LocationLookup` without a source-selection flag SHALL search both the structured location index and the text search index, mirroring the search JavaScout performs. Each result SHALL be marked with its source: structured index results as `idx`, text-index results as `txt`.

#### Scenario: Default run shows results from both sources

Given a database that has both a location index and a text search index
When the user runs LocationLookup for a search string without source-selection flags
Then the output contains results marked `idx` and results marked `txt`

#### Scenario: Same object from both sources appears once

Given a database where one object matches both the structured location search and the text search for the same string
When the user runs LocationLookup for that string
Then the object appears exactly once in the output

### Requirement: Source selection flags

`--structured-only` SHALL run only the structured location search and `--fulltext-only` SHALL run only the text index search. The flags SHALL be mutually exclusive.

#### Scenario: Structured-only run

Given a database with both a location index and a text search index
When the user runs LocationLookup with `--structured-only` for a search string
Then the output contains only results marked `idx`

#### Scenario: Fulltext-only run

Given a database with both a location index and a text search index
When the user runs LocationLookup with `--fulltext-only` for a search string
Then the output contains only results marked `txt`

### Requirement: Result limit applies to merged list

The `--limit` option SHALL limit the combined result list. Structured results SHALL be listed first and fulltext results SHALL fill the remaining slots. When fulltext hits are cut by the limit, the output SHALL report how many were truncated.

#### Scenario: Truncated fulltext hits are reported

Given a search that returns structured results and more fulltext hits than remaining slots in the limit
When the user runs LocationLookup with `--limit N`
Then the output contains at most N results and a line reporting the number of truncated fulltext hits

### Requirement: Missing text index handling

Without a text index in the database, `--fulltext-only` SHALL fail with an explanatory error. The default (both sources) run SHALL warn and continue with structured results only.

#### Scenario: Fulltext-only run without text index

Given a database without a text search index
When the user runs LocationLookup with `--fulltext-only`
Then the tool prints an error explaining that the text index is missing and exits with a non-zero status

#### Scenario: Default run without text index

Given a database without a text search index
When the user runs LocationLookup without source-selection flags
Then the tool prints a warning about the missing text index and still returns structured results marked `idx`

### Requirement: Fulltext availability depends on build

When LocationLookup is built without fulltext (MARISA) support, requesting fulltext search SHALL fail with an explanatory error instead of misbehaving.

#### Scenario: Fulltext-only run in a build without MARISA

Given a LocationLookup build compiled without MARISA support
When the user runs LocationLookup with `--fulltext-only`
Then the tool prints an error explaining that this build has no fulltext support and exits with a non-zero status
