null  :=
space := $(null) #
comma := ,

PKGS := unix,str,hashcons,hashset,mparser-pcre,ocamlgraph,dynlink
TAGS := debug,explain,annot,use_libsoundness
OCAMLDOC := ocamldoc -hide-warnings
INCLUDES := $(subst $(space),$(comma),$(strip $(wildcard src/*)))
OCB := ocamlbuild -use-ocamlfind -j 8 -ocamldoc "$(OCAMLDOC)" -pkgs $(PKGS) -tags $(TAGS) -Is $(INCLUDES)
OCAMLFORMAT_EXE := ocamlformat

OUR_DIRS := src/generic src/procedure src/seplog src/slsat src/soundness src/util src/while
OUR_SRCS := $(shell find $(OUR_DIRS) ! -name slinit.ml -name '*.ml' -or -name '*.mli')


ROOT := $(shell pwd)
DEFS := $(ROOT)/examples/sl.defs
export ROOT DEFS

TARGETS := \
	src/generic/checkproof \
	src/firstorder/fo_prove \
	src/seplog/sl_prove \
	src/seplog/sl_disprove \
	src/seplog/sl_modelcheck \
	src/slsat/sl_satcheck \
	src/slsat/slsat_expgen \
	src/while/while_prove  \
	src/while/while_abduce \
	src/procedure/procedure_prove

NATIVE_TARGETS := $(addsuffix .native,$(TARGETS)) 

BYTE_TARGETS := $(addsuffix .byte,$(TARGETS)) 

TOPLEVEL := src/seplog/sl.top

TEST_TARGETS := \
	tests/test_slterm_bug1.native \
	tests/test_slterm_bug2.native

.PHONY: all native byte toplevel check docs

byte: $(BYTE_TARGETS)

native: $(NATIVE_TARGETS)

toplevel: $(TOPLEVEL)

check: $(TEST_TARGETS)
	@for TST in _build/tests/test_*.native ; do $$TST ; done

docs: src/cyclist.docdir/index.html

all: native toplevel 

$(TEST_TARGETS): %.native:
	$(OCB) -no-links $@

%.native:
	$(OCB) $@

%.byte:
	$(OCB) $@

%.html:
	$(OCB) $@

%.top:
	$(OCB) $@

clean:
	$(OCB) -clean

%-tests:
	$(MAKE) -C benchmarks $*

.PHONY: fmt
fmt:
	@parallel $(OCAMLFORMAT_EXE) -i ::: $(OUR_SRCS)
