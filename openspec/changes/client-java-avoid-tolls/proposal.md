# client-java-avoid-tolls

## Why

`RoutingProfile.avoidTolls` is accepted by the JNI bridge and changes nothing: `CreateRoutingProfile`
answers it with a log line and builds a profile that has no way of knowing which roads charge. The shipped
`routing-profile` requirement already says the avoid flags are passed to the routing parameter, so the
capability is declared and not delivered. JavaScout shows the consequence to a user: its "Avoid tolls"
checkbox does nothing.

Unlike `avoidUnpaved` (change `client-java-avoid-unpaved`), the routing index records no toll signal at
all, so the capability cannot be delivered by the bridge alone: a way's `toll=yes` tag reaches a stored
database only through the importer, and a stored database reaches the user only through whoever builds it.

## What Changes

- A way tagged as charging a toll is recorded in the routing index, so a routing profile can see it.
- A route calculated with `avoidTolls` prices a toll way higher than the same way without the flag, and
  detours around it when a reasonable alternative exists.
- A database whose import recorded no toll information still loads and routes; with `avoidTolls` set it
  behaves exactly as before, because absence of the signal is never read as "free of charge" or as "toll".

## Impact

- Affected capability: `routing-profile`.
- Affected code: a way feature beside `GradeFeature` and its registration in the type configuration, the
  routing-index generator, the routing profile's cost function, and the bridge that builds the profile.
- Affected data: databases must be re-imported for the flag to have anything to act on. The app downloads
  prebuilt maps (the provider `osmscout.karry.cz` is the default), so the flag starts working for a user
  only when the provider rebuilds against a libosmscout carrying this change. Until then the bridge change
  is correct and inert — the honest state the issue asks for.
- Not affected: the index's file layout. The toll signal travels in the spare high bits of the grade byte,
  so a database imported before this change reads back as "no toll information" instead of misparsing.
