# Stream in C: `make test` esegue tutte le tappe, `make t1` / `t2` / `t3` una sola.
# I binari vanno fuori dal repo (in $TMPDIR/streams-c). -O0 di proposito: niente ottimizzazione delle
# chiamate in coda, così una ricorsione al posto di un ciclo si vede (stack overflow).
# AddressSanitizer + UBSan trovano accessi fuori dai limiti e puntatori a variabili locali già morte.

CC ?= cc
CFLAGS = -std=gnu11 -W -Wall -Wextra -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
OUT := $(shell echo $${TMPDIR:-/tmp/} | sed 's:/*$$::')/streams-c
export ASAN_OPTIONS = detect_stack_use_after_return=1:detect_leaks=0

TESTS = test_01_basi test_02_memo test_03_operazioni

.PHONY: test t1 t2 t3 clean

test: $(addprefix $(OUT)/,$(TESTS))
	@status=0; for t in $^; do $$t || status=1; done; exit $$status

t1: $(OUT)/test_01_basi
	@$<
t2: $(OUT)/test_02_memo
	@$<
t3: $(OUT)/test_03_operazioni
	@$<

$(OUT)/%: test/%.c stream.c stream.h test/test.h
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) test/$*.c stream.c -o $@ -lm

clean:
	rm -rf $(OUT)
