# Troubleshooting

## Install / upgrade

Download the latest release zip from
[Releases](https://github.com/sd88me/mpc-vst-cratedigger/releases), then:

```sh
scp -r Crate-Digger-<version> root@<device-ip>:/tmp/
ssh root@<device-ip> sh /tmp/Crate-Digger-<version>/install.sh
```

This stops MPC (save your project first), backs up `MPC.settings`, installs the plugin, and
restarts MPC. If Crate Digger is already installed, running this again just upgrades it in
place — no need to remove it first. Add `-y` to skip the confirmation prompt.

## Search shows an "error" status

The plugin screen only ever shows a short status word (`error`, `no_results`, ...) with no
detail. As of v1.06, the actual reason is written to a log file on the device.

### 1. Set a Discogs token (fixes most search errors)

Without a token, the device is capped at 25 Discogs requests/minute, and a single search can
use up to ~24 of those (one page-count probe, up to 3 page fetches, and up to 4 release
lookups per result) — so a second search shortly after, or another device on the same
network, easily trips a rate limit. A free personal token raises the limit to 60/min:

1. Get a token at <https://www.discogs.com/settings/developers>.
2. On the device, create/edit `/data/UserData/schwung/config/webstream_providers.json`:
   ```json
   { "providers": { "cratedig": { "token": "YOUR_DISCOGS_PERSONAL_TOKEN" } } }
   ```

### 2. Check the runtime log

```sh
ssh root@<device-ip>
cat /tmp/webstream-runtime.log
```

Look for lines starting with `search error:` — the most recent one is the actual cause.
Common ones:

| Log line contains | Meaning | Fix |
|---|---|---|
| `HTTP Error 429` | Rate-limited by Discogs | Add a token (above), or wait a minute between searches |
| `HTTP Error 401` | Token is missing/invalid | Re-check the token in `webstream_providers.json` |
| `network error: ...` | Device has no internet/DNS | Check the device's network connection |
| anything else | Not yet a known cause | Report it (see below) with the exact line |

If the log file doesn't exist yet, no search has failed since the last MPC restart — try
reproducing the issue, then check again.

## Reporting a search failure

Open an issue at <https://github.com/sd88me/mpc-vst-cratedigger/issues> with:
- The exact `search error:` line from `/tmp/webstream-runtime.log`.
- Whether a Discogs token is configured.
- The genre/style/decade/region/country filters in use at the time.

## Helping us debug "search stops responding" or "playback breaks after a new search"

These two are still open (see [ROADMAP.md](ROADMAP.md)) and haven't been reproduced yet off a
device, so the fastest way to move them forward is a log capture while reproducing one. As of
this version, every log line in both files below is timestamped (`[seconds.milliseconds]`), so
the two logs can be read together as one timeline of what happened and in what order.

1. Reproduce the issue:
   - **Search gets stuck**: trigger a slow/failing search (e.g. searching right after another
     one, or with a filter combo that returns nothing quickly), wait for the `error` status,
     then press SEARCH again and confirm it doesn't restart.
   - **Playback breaks after a new search**: start a track playing from one search's results,
     then run a new search (with the track still playing) and try to select a track from the
     new results.
2. Immediately after, grab both log files:
   ```sh
   ssh root@<device-ip> cat /tmp/webstream-runtime.log
   ssh root@<device-ip> cat /tmp/cratedigger_vst.log
   ```
3. Attach both (full, not just the tail) to an issue at
   <https://github.com/sd88me/mpc-vst-cratedigger/issues>, along with:
   - What you did, step by step, and roughly when (so it can be matched to the timestamps).
   - Whether a Discogs token is configured.

`/tmp/webstream-runtime.log` now also logs, for every search dispatch, whether it ran
immediately, queued behind one already running, or restarted a stuck daemon — and for every new
track selection, what the previous stream's state was (still resolving, still playing, etc.).
`/tmp/cratedigger_vst.log` logs every SEARCH press and every result tap from the plugin's own
side, with the same timestamp clock.
