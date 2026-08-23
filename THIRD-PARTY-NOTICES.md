# Third-Party Notices

Portal2HeadTracking bundles, statically links, or credits the third-party components
listed below. Each remains the property of its authors and is used under its own
licence. Where a licence requires the copyright notice, the conditions and the
disclaimer to accompany a binary distribution, the full text is reproduced here
verbatim, and this file ships at the root of every release ZIP we publish.

No Portal 2 code is used, reproduced or redistributed anywhere in this
repository. The one piece of Valve-owned material here is the README demo
clip, which stays out of both release ZIPs and is set out in full under
"Portal 2 footage" below.

| Component | Version | Licence | How it ships |
|-----------|---------|---------|--------------|
| Ultimate ASI Loader | v9.7.2 | MIT | Bundled verbatim in the installer ZIP |
| MinHook | `c3fcafdc1014` | BSD-2-Clause | Compiled into `Portal2HeadTracking.asi` |
| cameraunlock-core | 3465659888b2270addac9de0b2a728f59a00360c | MIT | Compiled into `Portal2HeadTracking.asi` |
| OpenTrack | n/a | ISC | Not bundled; UDP protocol interoperability only |

---

## Ultimate ASI Loader

Vendored at `vendor/ultimate-asi-loader/`, shipped in the installer ZIP and used as the
install-time source. Taken from the upstream release asset untouched; the
upstream licence file ships beside it at `vendor/ultimate-asi-loader/LICENSE`.

- Upstream: https://github.com/ThirteenAG/Ultimate-ASI-Loader
- Version: `v9.7.2`
- Commit: `ab722befd52581a34449b603926cfab476e66b05`
- SHA-256: `c7277e832f6f07af64903a99ecebab2936260cbf55eda70787c5d7b2d5b9fe60`

```
MIT License

Copyright (c) 2023 ThirteenAG

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## MinHook

Not vendored in this repository. CMake fetches it from upstream at configure
time, pinned to the commit SHA below rather than a mutable tag, and it is
compiled unmodified into `Portal2HeadTracking.asi`.

- Upstream: https://github.com/TsudaKageyu/minhook
- Commit: `c3fcafdc10146beb5919319d0683e44e3c30d537` (v1.3.4)

MinHook carries two copyright holders: Tsuda Kageyu for MinHook itself, and
Vyacheslav Patkov for the Hacker Disassembler Engine that MinHook's own
`src/hde/` is built from. Both notices appear below exactly as upstream ships
them.

```
MinHook - The Minimalistic API Hooking Library for x64/x86
Copyright (C) 2009-2017 Tsuda Kageyu.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

================================================================================
Portions of this software are Copyright (c) 2008-2009, Vyacheslav Patkov.
================================================================================
Hacker Disassembler Engine 32 C
Copyright (c) 2008-2009, Vyacheslav Patkov.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

-------------------------------------------------------------------------------
Hacker Disassembler Engine 64 C
Copyright (c) 2008-2009, Vyacheslav Patkov.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

---

## cameraunlock-core

Git submodule at `cameraunlock-core/`, compiled into `Portal2HeadTracking.asi`. Our own code,
MIT licensed, reproduced here so the notices are complete.

- Pinned commit: `3465659888b2270addac9de0b2a728f59a00360c`

```
MIT License

Copyright (c) 2026 CameraUnlock

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## OpenTrack

Not bundled and not linked. This mod implements the OpenTrack UDP pose datagram
layout so that OpenTrack (https://github.com/opentrack/opentrack, ISC licence)
and compatible trackers can drive it. No OpenTrack code, headers or binaries
are copied, linked or redistributed, so its licence triggers no notice
obligation here. It is credited because the wire format is its work.

---

## Portal 2 and the Source Engine

Portal 2 and the Source Engine are (C) Valve Corporation. This mod is
unaffiliated with and unendorsed by Valve, and requires a legitimately
purchased copy of the game on Steam.

**No Valve code ships here, and neither release ZIP contains any Valve
material at all.** There is no Valve source, no Source SDK file or header, no
decompiler or disassembler output, no extracted asset, and no game data file.
The single exception anywhere in the repository is `assets/readme-clip.gif`,
ten seconds of recorded gameplay used to show what the mod does; it is
Valve's, it is described under "Portal 2 footage" below, and the packaging
scripts ship no part of `assets/`. Nothing here helps anyone play Portal 2
without owning it, and the mod refuses to run against a game binary it does
not recognise.

What the mod does hold about the game is a short table of numbers in
`src/builds/`: the relative address of one function in `client.dll`, the byte
offsets of a few fields in the view struct that function receives, and a PE
header fingerprint used to confirm which shipped build is running. Those are
measurements of the user's own installed copy, recorded so the mod can
interoperate with it, and they carry none of the game's expression.

`src/source_math.cpp` is our own implementation of the engine's camera angle
conventions, written from their observable behaviour so that an injected view
composes the way the renderer's does. Those formulas are the only ones that
produce a matching result, which makes them facts rather than anyone's
expression; no Valve source was copied to obtain them.

---

## Portal 2 footage

- **File:** `assets/readme-clip.gif` - roughly ten seconds of ordinary
  gameplay, 800x450, no audio.
- **Rights holder:** Valve Corporation, together with the rights holders of any
  third-party marks visible in frame.
- **Usage:** recorded from the game running with this mod, captured on a
  legitimately purchased copy, and embedded in the README so a reader can see
  what the mod does before installing it. It is not marketing material, not a
  cutscene, and not a re-upload of anything Valve published.
- **Distribution:** kept in this repository only. The packaging scripts copy no
  part of `assets/`, so the clip is in neither release ZIP and in nothing the
  launcher deploys.
- **Licence:** none is granted or implied. This material is not covered by the
  MIT licence in `LICENSE`, and nothing here permits anyone to reuse it. Valve,
  or any other rights holder, who would rather it were not published: open an
  issue or reach us on Discord and it comes down.

---

## Portal 2

Portal 2 and all related names, logos, characters and marks are trademarks of
their respective owners. They are used here only to identify the game this mod
applies to, which is nominative use and not a claim of any right in them. This
project is an unofficial, fan-made modification. It is not affiliated with,
endorsed by, or sponsored by the game's developers, its publishers, its engine
vendor, or any other rights holder. Neither the mod nor either release ZIP
redistributes any game code, extracted asset or proprietary DLL, and the mod
requires a legitimately purchased copy of the game. Any engine structure offsets, function addresses or byte patterns
referenced in the source were derived by the authors through independent
analysis of a legitimately owned copy. They are factual measurements recorded
as numbers; no decompiled or disassembled game code is stored in this
repository.
