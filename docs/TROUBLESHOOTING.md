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
