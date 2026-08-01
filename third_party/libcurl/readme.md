# libcurl (prebuilt, MinGW x86_64) — Windows only

Unmodified binary distribution — nothing here is ours, nothing here is
patched. Consumers include `<curl/curl.h>` and link against `lib/`.

**This is the Windows half of the story only.** These are MinGW
`x86_64-w64-mingw32` archives; they cannot link on Linux. Linux builds
use the distro's own libcurl via `pkg-config --static libcurl`
(package `libcurl4-openssl-dev` or `libcurl-devel`), so nothing needs
vendoring there. See `dbjobserve/Makefile` for the `$(OS)` split and
`dbjobserve/general_design.md` for why only one side is vendored.

- Version: curl 8.21.0_6, `win64-mingw` build
- Source: https://curl.se/windows/ (the curl-for-win project)
- Archive: `curl-8.21.0_6-win64-mingw.zip`
- SHA256: `0c119d394bab91ecd97b64791f24bddc247d08840431e0045bcb92bba3275665`
  (verified against the publisher's `.zip.txt` manifest at install time)
- License: `COPYING.txt` (curl license, MIT/X-derived)

`win64-mingw` is the x86_64 build. The `win64a-mingw` archive on the
same page is ARM64 — not what this toolchain (`x86_64-w64-mingw32`)
wants.

## Layout

- `include/curl/` — public headers
- `lib/` — `libcurl.a` (static) plus its static dependencies
  (LibreSSL, nghttp2/3, ngtcp2, libssh2, brotli, zstd, zlib, libpsl),
  and `libcurl.dll.a` (import lib for the DLL)
- `BUILD-MANIFEST.txt` — exact component versions in this build

The upstream archive's `bin/` directory was **deliberately not
vendored**: it holds `libcurl-x64.dll`, `curl.exe`, `trurl.exe` and
`curl-ca-bundle.crt`, none of which a static build uses, and together
they are several MB of git history for nothing. Re-download the
archive named above if you ever want the DLL or the CLI tools.

## Linking, statically

This is the arrangement `dbjobserve/Makefile` uses; see that file for
the working command line. Two things are easy to get wrong:

1. **Name `lib/libcurl.a` by path, not `-lcurl`.** Both `libcurl.a` and
   `libcurl.dll.a` sit in `lib/`, and the linker prefers the import
   lib. You get a clean build that dies at startup with `0xC0000135`
   (missing `libcurl-x64.dll`) rather than a link error.
2. **`-DCURL_STATICLIB` is a header-only switch.** It controls
   `__declspec(dllimport)` in `curl.h`; it has no say in which archive
   the linker picks. Both are needed.

Link order is static archives first, Windows system libraries last.
`-lsecur32` (SSPI, for `InitSecurityInterfaceA`) and `-liphlpapi`
(`if_nametoindex`) are required by the static build and are the two
that are usually forgotten.

Linking against the DLL instead is fine — `-L lib -lcurl` without
`-DCURL_STATICLIB` — but then `bin/libcurl-x64.dll` has to be on
`PATH` or beside the `.exe`.

## CA certificates

The static build verifies peers against the Windows certificate store
via Schannel/SSPI, so no extra setup is needed for HTTPS. A live
request to `https://services.jobserve.com` was verified working at
install time. If an explicit PEM bundle is ever wanted
(`CURLOPT_CAINFO`), `curl-ca-bundle.crt` is in the upstream archive's
`bin/`, which is not vendored here — see Layout above.

---
2026-08-01  dbj@dbj.org
