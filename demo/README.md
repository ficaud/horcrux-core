# Horcrux · WASM Demo

Shamir's Secret Sharing (SSS) over GF(256) — compiled to WebAssembly and running entirely in your browser.

Secret data **never leaves your machine**. All computations happen client-side via the WASM module compiled from `src/sss.c`.

## Usage

**Split** — enter any text (up to 100 characters) and get 5 shares.  
**Reconstruct** — paste shares in `x:hex` format (e.g. `3:60b5b9cfc025`) to recover the original secret.

The threshold is 3: any 3 out of 5 shares can reconstruct the secret.

## Build locally

Requires [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
make -C demo clean all
```

Or with Docker:

```bash
docker run --rm -v $(pwd)/demo:/src emscripten/emsdk:latest make -C /src clean all
```

## How it works

- `../src/sss/sss.c` / `../src/sss/sss.h` — pure C implementation of Shamir's Secret Sharing over GF(256) using the AES irreducible polynomial (0x11B)
- `src/main.c` — Emscripten glue exposing `sss_split_wasm` and `sss_combine_wasm`
- `scripts/app.js` — UI logic bridging WASM memory and the DOM
- Built by a GitHub Action on every push and deployed to **GitHub Pages**
