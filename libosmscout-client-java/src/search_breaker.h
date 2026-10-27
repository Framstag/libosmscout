/*
 * search_breaker.h — the search-request lifecycle: one slot per search surface,
 * one generation per request, and the abort predicate that unifies "superseded"
 * with "cancelled" (openspec change
 * `client-java-search-request-lifecycle`).
 *
 * Why a generation and not a cancel flag: the caller (a Kotlin coroutine) can be
 * cancelled, but the JNI frame it is blocked in has no suspension point, so its
 * cancellation handler runs only after that call returned — by which time the
 * successor search may already own the slot. A cancellation that cannot name the
 * request it means would then abort the *live* search, which is the defect the
 * existing parameterless cancel has. With a generation per request:
 *
 *   aborted(gen) == gen is not a live request any more
 *
 * A live request leaves the set when a newer request of its slot begins
 * (supersede), when it is cancelled, or when it ends — so one predicate covers
 * every way a search stops, and a late cancellation of a superseded generation
 * is a no-op by construction.
 *
 * Slots keep supersede inside one surface: the phone's places search, the POI
 * search, the route panel's field and the car's search template each own one
 * slot. In a projection session both the phone and the car are live in one
 * process, so the car starting a search must not abort the phone's.
 *
 * Kept dependency-free on purpose (no libosmscout includes, no JNI) so it can be
 * unit-tested on the host. The JNI bridge adapts it to `osmscout::Breaker` with a
 * thin adapter inside OSMScoutClient.cpp; this file only counts generations and
 * tracks which requests are live.
 */
#ifndef NAVIVEYLIN_SEARCH_BREAKER_H
#define NAVIVEYLIN_SEARCH_BREAKER_H

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace naviveylin {

/** Slot names. One search surface owns one slot. */
inline constexpr const char *kSearchSlotPlaces = "places";
inline constexpr const char *kSearchSlotPoi    = "poi";
inline constexpr const char *kSearchSlotRoute  = "route";
inline constexpr const char *kSearchSlotCar    = "car";
/** The car's POI results screen: its own surface, so it keeps its own slot. */
inline constexpr const char *kSearchSlotCarPoi = "car-poi";

/**
 * Tracks one live search request per slot.
 *
 * All four operations take the mutex for a map lookup or two and never hold it
 * across the search itself: the abort poll inside a database walk is a single
 * lookup. Generations are unique across slots, so a generation alone identifies
 * its request — which is all the JNI boundary can carry (a `jlong`).
 */
class SearchBreakerRegistry
{
public:
  using Generation = std::uint64_t;

  /** The sentinel for "no request"; real generations start at 1. */
  static constexpr Generation kNoGeneration = 0;

private:
  /** Generation -> slot, for every request that is still live. */
  std::map<Generation,std::string> live;
  /** Slot -> its current generation. */
  std::map<std::string,Generation> current;
  /** Source of unique generations, monotonic across slots. */
  Generation                       lastGeneration{kNoGeneration};
  mutable std::mutex               mutex;

  /**
   * Drops a request from the live set, if it is in it. Returns whether it was
   * there; the caller holds the mutex.
   */
  bool Drop(Generation generation)
  {
    auto it = live.find(generation);

    if (it == live.end()) {
      return false;
    }

    auto slotIt = current.find(it->second);

    if (slotIt != current.end() && slotIt->second == generation) {
      current.erase(slotIt);
    }

    live.erase(it);

    return true;
  }

public:
  /**
   * Begins a search for `slot`: the slot's predecessor, if any, is superseded and
   * a fresh generation is returned as the slot's current one. An unknown slot
   * name is a slot of its own — the app uses the four names above, and a typo is
   * a separate surface rather than a shared one.
   */
  Generation Begin(const std::string &slot)
  {
    std::scoped_lock guard(mutex);

    auto slotIt = current.find(slot);

    if (slotIt != current.end()) {
      Drop(slotIt->second);
    }

    Generation generation = ++lastGeneration;

    live[generation]    = slot;
    current[slot]       = generation;

    return generation;
  }

  /**
   * Cancels the request with this generation and reports whether it was live.
   * `false` means a newer request of its slot had already replaced it (or it had
   * ended), so nothing was aborted — the property that makes a late cancellation
   * harmless.
   */
  bool Cancel(Generation generation)
  {
    std::scoped_lock guard(mutex);

    return Drop(generation);
  }

  /**
   * Whether the request with this generation is aborted: true once a newer
   * request of its slot began, once it was cancelled, or once it ended. A
   * generation that is still live and not cancelled is the only non-aborted case.
   */
  bool IsAborted(Generation generation) const
  {
    std::scoped_lock guard(mutex);

    return live.find(generation) == live.end();
  }

  /**
   * The slot of a live request, for the diagnostics line. Empty once the request is gone (superseded,
   * cancelled or ended) — which is why a caller that reports its slot reads it before [End].
   */
  std::string SlotOf(Generation generation) const
  {
    std::scoped_lock guard(mutex);

    auto it = live.find(generation);

    if (it == live.end()) {
      return std::string();
    }

    return it->second;
  }

  /**
   * Ends a finished search, so its slot is idle again and a later cancellation of
   * its generation reports `false` instead of "aborting" a search that is over.
   *
   * A caller that reports whether its search was aborted must read
   * [IsAborted] *before* ending it.
   */
  void End(Generation generation)
  {
    std::scoped_lock guard(mutex);

    Drop(generation);
  }

  /**
   * Number of live requests, for tests and diagnostics. Never more than the
   * number of slots that started a search.
   */
  std::size_t LiveCount() const
  {
    std::scoped_lock guard(mutex);

    return live.size();
  }
};

} // namespace naviveylin

#endif // NAVIVEYLIN_SEARCH_BREAKER_H
