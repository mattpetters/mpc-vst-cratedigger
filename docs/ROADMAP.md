# Roadmap

User feedback triage, from real device reports. Ordered by what's actionable now vs. what
needs more investigation or design first.

## 1. Results page always "1/1", only 5 tracks shown — DONE

`yt_stream_plugin.c:1355` requests `SEARCH_MAX_RESULTS` (20) results from the daemon, but
`src/bin/yt_dlp_daemon.py`'s `cratedig_search()` hardcoded `if count > 5: count = 5` — capping
every search at 5 releases regardless of what was asked for. With the skin's results list at
8 rows/page (`SLOTS=8` in `vst/cratedigger_vst.cpp`), that meant the page stepper never had
more than one page to show.

Fixed: raised the cap to 16 (2 full pages). Kept the per-search Discogs API cost bounded
independently of `count` (`max_release_lookups = min(count * 4, 20)` in
`get_random_releases()`), since each candidate release needs its own `/releases/{id}` call and
the unauthenticated rate limit is only 25/min (see `docs/TROUBLESHOOTING.md`) — raising the
results count shouldn't multiply the rate-limit risk by the same factor.

## 2. Scroll wheel feels "funky" moving through filters

`vst/cratedigger_vst.cpp:296-306`: a Q-Link/scroll-wheel turn always steps the stepper by
exactly ±1, regardless of how far the wheel moved — it only jumps further when the computed
position lands within 0.001 of an exact multiple. Fast scrolling sends bigger position deltas
per callback, so it feels inconsistent (sometimes 1 step, occasionally more).

Next: repro on-device with Q-Link turn speed logged, then likely accumulate fractional turn
distance across calls instead of snapping per-call.

## 3. Long queue time → error → SEARCH again does nothing → stuck until new instance

`yt_stream_plugin.c`'s queued-search protocol (`start_search_async`/`search_thread_main`,
`queued_search_pending`) looks correct on read-through — a queued request should auto-fire when
the in-flight one finishes, even on error. Either there's a race not yet found, or the
persistent `yt_dlp_daemon.py` subprocess itself wedges after a timeout/error and every
subsequent request blocks forever (would better explain "never searches again" than the
thread-queue logic alone).

Next: on-device repro with `/tmp/webstream-runtime.log` captured through a full stuck cycle.

## 4. Playing a track, then searching again "breaks" selection until a new instance

Needs tracing through what state `play_result`/`stream_url` is in when a new `cratedig_filter`
search lands mid-playback, and whether the stream needs an explicit stop first. The reporting
user's own hunch ("stop before searching?") is a plausible workaround to confirm — if true,
either enforce it automatically (stop stream on new search) or fix the underlying conflict.

May share a root cause with #3 (search/stream state left dirty across actions) — worth
investigating together.

## 5. Buffer-time countdown before playback starts

New feature: give the user a window to switch to the sampler and arm record before audio
starts, up to 5s. `search_elapsed_ms` is precedent for this kind of timing instrumentation;
would need an equivalent for the resolve+buffer phase (`resolve_stream_url` /
`start_stream_resolved`), exposed as a new readout param.

Needs a design decision: an on-screen countdown, or just a fixed delay before playback starts.

## 6. Transport-sync button

New feature: play a buffered track in time with MPC's own transport (press Play on the device
to start it), to assist with resampling. Would use `audioMasterGetTime` (already used
elsewhere per `mpc-vst-plugins`' docs/NOTES.md) to detect MPC's own Play and gate playback
start on it instead of the SEARCH/tap trigger.

Biggest, most design-y item on this list — lowest urgency, scope properly (ideally with the
user who suggested it) before building.
