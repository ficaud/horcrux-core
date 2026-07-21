# Makefile — Build SSS WASM demo
#
# Requires Emscripten SDK:
#   docker run --rm -v $(PWD):/src emscripten/emsdk make -C /src
#
# References the parent project sss.c directly (no copy).

EMCC = emcc
CFLAGS = -O3 -flto
WASM_FLAGS = \
	-s MODULARIZE=1 \
	-s EXPORT_NAME=SSS \
	-s EXPORTED_FUNCTIONS=_wasm_set_seed,_sss_split_wasm,_sss_combine_wasm,_malloc,_free \
	-s EXPORTED_RUNTIME_METHODS=ccall,cwrap,getValue,setValue \
	-s NO_EXIT_RUNTIME=1 \
	-s FILESYSTEM=0 \
	--no-entry

HORCRUX_SSS_DIR = ../src/sss
HORCRUX_SSS = $(HORCRUX_SSS_DIR)/sss.c
SRC = src/main.c src/sys_rand_stub.c $(HORCRUX_SSS)
INC = -Isrc -Isrc/stubs -I$(HORCRUX_SSS_DIR)
OUT_WASM = sss.wasm
OUT_JS = sss.js

.PHONY: all clean

all: $(OUT_JS) $(OUT_WASM)

$(OUT_JS) $(OUT_WASM): $(SRC)
	$(EMCC) $(CFLAGS) $(INC) $(WASM_FLAGS) -o $(OUT_JS) $(SRC)

clean:
	rm -f $(OUT_JS) $(OUT_WASM)
