# Ultimate ASI Loader (vendored)

Bundled copy of Ultimate ASI Loader (x86), the install-time source of truth.
install.cmd copies it straight out of here and never reaches out to the network.
Refresh manually with `pixi run update-deps`, then commit.

## Snapshot

- Upstream: https://github.com/ThirteenAG/Ultimate-ASI-Loader
- Tag: `v9.7.2`
- Commit: `ab722befd52581a34449b603926cfab476e66b05`
- Asset: `Ultimate-ASI-Loader.zip`
- Asset URL: https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.2/Ultimate-ASI-Loader.zip
- dinput8.dll SHA-256: `c7277e832f6f07af64903a99ecebab2936260cbf55eda70787c5d7b2d5b9fe60`
- Fetched at: 2026-06-04T15:50:13.2487585+01:00

`dinput8.dll` is extracted from the upstream x86 zip untouched. install.cmd copies it
to <game>\bin\winmm.dll, the proxy slot Portal 2 loads ASI plugins through.
