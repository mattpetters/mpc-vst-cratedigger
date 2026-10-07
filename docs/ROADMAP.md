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

## 2. Scroll wheel feels "funky" moving through filters — DONE

`vst/cratedigger_vst.cpp`'s stepper `setParameter` mixed two behaviors: a Q-Link/scroll-wheel
value that landed within 0.001 of an exact step boundary jumped straight there (possibly by
more than one step), while everything else nudged by exactly ±1 regardless of how far the
value had moved — same physical motion, inconsistent step size.

Fixed: replaced both branches with one deterministic rule — land on `round(pos)`, clamped to
the stepper's range. A bigger turn now reliably moves further; a small one reliably moves one
step.

## 3. Long queue time → error → SEARCH again does nothing → stuck until new instance —
INVESTIGATING (logging in place)

`yt_stream_plugin.c`'s queued-search protocol (`start_search_async`/`search_thread_main`,
`queued_search_pending`) looks correct on read-through — a queued request should auto-fire when
the in-flight one finishes, even on error. Either there's a race not yet found, or the
persistent `yt_dlp_daemon.py` subprocess itself wedges after a timeout/error and every
subsequent request blocks forever (would better explain "never searches again" than the
thread-queue logic alone).

`start_search_async()` now logs its thread_valid/thread_running/queued state and which path it
took (run now / queue / restart) to `/tmp/webstream-runtime.log`, and every log line in both
that file and `vst/cratedigger_vst.cpp`'s `/tmp/cratedigger_vst.log` is timestamped so the two
can be read as one timeline. Next: a user (or us) reproduces it and attaches both logs — see
`docs/TROUBLESHOOTING.md`'s "Helping us debug" section.

## 4. Playing a track, then searching again "breaks" selection until a new instance —
INVESTIGATING (logging in place)

Needs tracing through what state `play_result`/`stream_url` is in when a new `cratedig_filter`
search lands mid-playback, and whether the stream needs an explicit stop first. The reporting
user's own hunch ("stop before searching?") is a plausible workaround to confirm — if true,
either enforce it automatically (stop stream on new search) or fix the underlying conflict.

The `stream_url` setter now logs the previous stream's state (pipe/eof/paused) before it's
replaced, and the wrapper logs every SEARCH press and result tap with the engine's
search_status/stream_status at that moment — same repro process as #3, and may share a root
cause with it (search/stream state left dirty across actions).

## 5. Buffer-time countdown before playback starts — SKIPPED for now

New feature: give the user a window to switch to the sampler and arm record before audio
starts, up to 5s. `search_elapsed_ms` is precedent for this kind of timing instrumentation;
would need an equivalent for the resolve+buffer phase (`resolve_stream_url` /
`start_stream_resolved`), exposed as a new readout param.

Needs a design decision: an on-screen countdown, or just a fixed delay before playback starts.

## 6. Transport-sync button — DONE (v1.1)

New feature: play a buffered track in time with MPC's own transport (press Play on the device
to start it), to assist with resampling. Would use `audioMasterGetTime` (already used
elsewhere per `mpc-vst-plugins`' docs/NOTES.md) to detect MPC's own Play and gate playback
start on it instead of the SEARCH/tap trigger.

Biggest, most design-y item on this list — lowest urgency, scope properly (ideally with the
user who suggested it) before building.

## 7. Saved MPC preset errors when used to search — HARDENED (v1.1)

Report: saving a filter setup as an MPC preset, then searching from that preset, errors.
The preset is our `effGetChunk`/`effSetChunk` string (`g=;s=;d=;r=;c=;gain=`, dimension
*indexes* only) in `vst/cratedigger_vst.cpp`. Leads: `effSetChunk` calls `set_dim` for genre,
style, decade, region and country in one go, but style and country depend on genre and region,
so a stored style/country index may be applied against the wrong (or not yet loaded) option
list; and nothing checks that the restored indexes are in range for the current lists. Next:
reproduce with `/tmp/cratedigger_vst.log`, and compare the filter values the search actually
sends after a chunk restore with what the UI shows.

## 8. Jog wheel: one click takes 1-2 s to register — DONE (v1.1)

Report: a single detent in either direction is slow, but fast spinning is fine, so an
accidental landing costs a wait to scroll away. Not fixed by #2, which only changed the step
size. Leads: `setParameter` holds `p->lock` while `stepper_set` runs, and the worker holds
the same lock during `render_block`; the display refresh only runs every 200 ms in
`worker_main`. Next: time `stepper_set`, check whether a dependent-list refresh
(genre → style, region → country) blocks, and consider refreshing the display straight after
a step instead of waiting for the tick.

## 9. Feedback from the same report that is already tracked

Scroll-wheel feel (#2), the results page stuck at 1/1 (#1), search errors and being stuck
after an error (#3), selection breaking after playback (#4) and the buffer countdown (#5) are
already listed above.

## 10. Search by text (title/artist/label) — DONE

New SEARCH tab: type a term and search Discogs by text as well as by facet, for
"get me this record" rather than a lucky dip.

- **Where the term goes**: into the same `cratedig_filter` JSON the steppers
  already build (a `query` field), so the engine core needed no change at all —
  `src/bin/yt_dlp_daemon.py` sends it as Discogs' `q=`, combined with whatever
  genre/style/country/year facets are set (verified live 2026-10-07:
  `q=roy ayers&genre=Jazz&year=1976` narrows 666 items to 50).
- **How it is typed**: the host has no text entry (mpc-vst-plugins docs/NOTES.md
  — "No text entry on the page"), so the box is built from a skin `popup` key
  grid: 43 keys plus one deliberately dead last cell. MPC sends nothing when the
  option it believes is selected is tapped, so the wrapper reports *that* dead
  cell as the grid's current value — no real key can ever be the "already
  selected" one, and typing the same letter twice works. The buffer itself is
  `vst/query_edit.h`, which has a plain host test (`vst/test_query_edit.c`,
  21 checks) since it is logic rather than glue.
- **Ranking**: a text search keeps Discogs' relevance order, walks the top of the
  result list rather than sampling random pages, drops duplicate video URLs,
  ignores `exclude_ids` (the same term twice must give the same releases) and
  caps release lookups near `count` (fewer Discogs calls than the filter path,
  which matters against the unauthenticated 25/min limit).
- **MORE BY ARTIST**: re-searches the artist of the last result row tapped — no
  typing, same plumbing.
- **Not quietly narrowed**: `FILTERS IN USE` on the SEARCH tab names the facets
  the next search will send, since a text query still combines with them.
- Empty text → SEARCH is exactly the old filter behaviour, byte for byte.

Still to verify on a device: the key grid's drawn list and its dead cell, and
the flag round-trip that closes it (all read off mpc-vst-plugins' popup
mechanism at the pinned tools ref, not yet seen on hardware).
