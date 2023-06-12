TEST_EXE=./soundness

CFLAGS:=-xc++ -std=c++17

TIMEOUT := $(if $(TIMEOUT),$(TIMEOUT),60)

define test_success
	echo "TEST: $(1), FLAGS=$$FLAGS"
	(timeout $(TIMEOUT) $(TEST_EXE) $(1); EXIT_CODE=$$?; if [ $$EXIT_CODE -eq 0 ]; then echo "TEST PASSED"; elif [ $$EXIT_CODE -eq 124 ]; then echo "TIMEOUT"; else echo "TEST NOT PASSED"; fi; exit 0)
endef

define test_failure
	echo "TEST: $(1), FLAGS=$$FLAGS"
	(timeout $(TIMEOUT) $(TEST_EXE) $(1); EXIT_CODE=$$?; if [ ! $$EXIT_CODE -eq 0 ] && [ ! $$EXIT_CODE -eq 124 ]; then echo "TEST PASSED"; elif [ $$EXIT_CODE -eq 124 ]; then echo "TIMEOUT"; else echo "TEST NOT PASSED"; fi; exit 0)
endef

all: soundness tests

soundness: main.o sloped_relation.o heighted_graph.o  graph_algorithms.o automata.o
	g++ -pthread -lspot -g -o soundness main.o sloped_relation.o heighted_graph.o  graph_algorithms.o

main.o: main.cpp ../heighted_graph.hpp 
	g++ ${CFLAGS} -I ../ -L/opt/lib -g -c main.cpp -o main.o -lspot

automata.o : ../automata.c
	g++ ${CFLAGS} -I ../ -L/opt/lib -g -c ../automata.c -o automata.o -lspot

heighted_graph.o: ../heighted_graph.hpp ../heighted_graph.c ../sloped_relation.hpp ../types.c
	g++ ${CFLAGS} -pthread -I ../ -L/usr/lib -L/opt/lib -g -c ../heighted_graph.c -o heighted_graph.o

sloped_relation.o: ../sloped_relation.c ../sloped_relation.hpp ../types.c
	g++ ${CFLAGS} -I ../ -L/opt/lib -g -c ../sloped_relation.c -o sloped_relation.o

graph_algorithms.o: ../graph_algorithms.c ../graph_algorithms.hpp ../sloped_relation.hpp
	g++ ${CFLAGS} -I ../ -L/opt/lib -g -c ../graph_algorithms.c -o graph_algorithms.o

.PHONY: clean tests test-suite-rel test-suite-sd test-suite-xsd tests-rel tests-set tests-no-opts tests-ff tests-scc tests-idem tests-min tests-ff-min tests-ff-scc tests-ff-idem tests-min-scc tests-ff-min-scc tests-1-ff tests-1-scc tests-1-idem tests-1-min tests-1-ff-min tests-1-ff-scc tests-1-ff-idem tests-1-min-scc tests-1-ff-min-scc tests-sd tests-xsd

.SILENT: tests tests-rel tests-set tests-sd tests-xsd test-suite-rel test-suite-sd test-suite-xsd test_success test_failure tests-no-opts tests-ff tests-scc tests-min tests-idem tests-ff-scc tests-min-scc tests-ff-idem

clean: 
	rm -f ./*.o soundness

tests: tests-rel tests-set

tests-set: tests-sd tests-xsd

tests-rel: tests-no-opts tests-ff tests-scc tests-idem tests-min tests-ff-min tests-ff-scc tests-ff-idem tests-min-scc tests-ff-min-scc tests-1-ff tests-1-scc tests-1-idem tests-1-min tests-1-ff-min tests-1-ff-scc tests-1-ff-idem tests-1-min-scc tests-1-ff-min-scc

tests-no-opts:
	echo "NO OPTIMSATIONS";
	export FLAGS=A; make -s test-suite-rel
tests-ff:
	echo "FAST-FAIL"
	export FLAGS=Af; make -s test-suite-rel
tests-scc:
	echo "SCC LOOP CHECK"
	export FLAGS=As; make -s test-suite-rel
tests-idem:
	echo "IDEMPOTENT RELATIONS ONLY"
	export FLAGS=Ai; make -s test-suite-rel
tests-min:
	echo "MINIMAL CCL"
	export FLAGS=Am; make -s test-suite-rel
tests-ff-min:
	echo "FAST-FAIL, MINIMAL CCL"
	export FLAGS=Afm; make -s test-suite-rel
tests-ff-scc:
	echo "FAST-FAIL, SCC LOOP CHECK"
	export FLAGS=Afs; make -s test-suite-rel
tests-ff-idem:
	echo "FAST-FAIL, IDEMPOTENT RELATIONS ONLY"
	export FLAGS=Afi; make -s test-suite-rel
tests-min-scc:
	echo "MINIMAL CCL, SCC LOOP CHECK"
	export FLAGS=Ams; make -s test-suite-rel
tests-ff-min-scc:
	echo "MINIMAL CCL, SCC LOOP CHECK"
	export FLAGS=Afms; make -s test-suite-rel
tests-1-ff:
	echo "FAST-FAIL"
	export FLAGS=f; make -s test-suite-rel
tests-1-scc:
	echo "SCC LOOP CHECK"
	export FLAGS=s; make -s test-suite-rel
tests-1-idem:
	echo "IDEMPOTENT RELATIONS ONLY"
	export FLAGS=i; make -s test-suite-rel
tests-1-min:
	echo "MINIMAL CCL"
	export FLAGS=m; make -s test-suite-rel
tests-1-ff-min:
	echo "FAST-FAIL, MINIMAL CCL"
	export FLAGS=fm; make -s test-suite-rel
tests-1-ff-scc:
	echo "FAST-FAIL, SCC LOOP CHECK"
	export FLAGS=fs; make -s test-suite-rel
tests-1-ff-idem:
	echo "FAST-FAIL, IDEMPOTENT RELATIONS ONLY"
	export FLAGS=fi; make -s test-suite-rel
tests-1-min-scc:
	echo "MINIMAL CCL, SCC LOOP CHECK"
	export FLAGS=ms; make -s test-suite-rel
tests-1-ff-min-scc:
	echo "MINIMAL CCL, SCC LOOP CHECK"
	export FLAGS=fms; make -s test-suite-rel

test-suite-rel: soundness
	$(call test_success, graph_1)
	$(call test_failure, graph_2)
	$(call test_success, graph_3)
	$(call test_success, graph_4)
	$(call test_success, graph_5)
	$(call test_failure, graph_6)
	$(call test_failure, graph_7)
	$(call test_failure, graph_8)
	$(call test_failure, graph_8a)
	$(call test_success, graph_8b)
	$(call test_failure, graph_9)
	$(call test_failure, graph_9a)
	$(call test_failure, graph_9b)
	$(call test_success, graph_10)
	$(call test_failure, graph_11)
	$(call test_failure, graph_12)
	$(call test_success, graph_13)
	$(call test_failure, graph_14)
	$(call test_failure, graph_15)
	$(call test_success, graph_16.00)
	$(call test_success, graph_16.01)
	$(call test_success, graph_16.02)
	$(call test_success, graph_16.03)
	$(call test_success, graph_16.04)
	$(call test_success, graph_16.05)
	$(call test_success, graph_16.06)
	$(call test_success, graph_16.07)
	$(call test_success, graph_16.08)
	$(call test_success, graph_16.09)
	$(call test_success, graph_16.10)
	$(call test_failure, graph_17)
	$(call test_success, graph_18)
	$(call test_success, graph_19)
	$(call test_failure, graph_20a)
	$(call test_failure, graph_20b)
	$(call test_failure, graph_21)

tests-sd:
	echo "SPRENGER-DAM"
	export FLAGS=D; make -s test-suite-sd

tests-xsd:
	echo "EXTENDED SPRENGER-DAM"
	export FLAGS=X; make -s test-suite-xsd

test-suite-sd: soundness
	$(call test_success, graph_1)
	$(call test_failure, graph_2)
	$(call test_success, graph_3)
	$(call test_success, graph_4)
	$(call test_success, graph_5)
	$(call test_failure, graph_6)
	$(call test_failure, graph_7)
	$(call test_failure, graph_8)
	$(call test_failure, graph_8a)
	$(call test_success, graph_8b)
	$(call test_failure, graph_9)
	$(call test_failure, graph_9a)
	$(call test_failure, graph_9b)
	$(call test_success, graph_10)
	$(call test_failure, graph_11)
	$(call test_failure, graph_12)
	$(call test_failure, graph_13) # succeeds with relational
	$(call test_failure, graph_14)
	$(call test_failure, graph_15)
	$(call test_success, graph_16.00)
	$(call test_success, graph_16.01)
	$(call test_success, graph_16.02)
	$(call test_success, graph_16.03)
	$(call test_success, graph_16.04)
	$(call test_success, graph_16.05)
	$(call test_success, graph_16.06)
	$(call test_success, graph_16.07)
	$(call test_success, graph_16.08)
	$(call test_success, graph_16.09)
	$(call test_success, graph_16.10)
	$(call test_failure, graph_17)
	$(call test_success, graph_18)
	$(call test_failure, graph_19)
	$(call test_failure, graph_20a)
	$(call test_failure, graph_20b)
	$(call test_failure, graph_21)

test-suite-xsd: soundness
	$(call test_success, graph_1)
	$(call test_failure, graph_2)
	$(call test_success, graph_3)
	$(call test_success, graph_4)
	$(call test_success, graph_5)
#	$(call test_failure, graph_6)
	$(call test_failure, graph_7)
	$(call test_failure, graph_8)
	$(call test_failure, graph_8a)
	$(call test_success, graph_8b)
#	$(call test_failure, graph_9)
	$(call test_failure, graph_9a)
	$(call test_failure, graph_9b)
	$(call test_success, graph_10)
	$(call test_failure, graph_11)
	$(call test_failure, graph_12)
	$(call test_success, graph_13)
	$(call test_failure, graph_14)
	$(call test_failure, graph_15)
	$(call test_success, graph_16.00)
	$(call test_success, graph_16.01)
	$(call test_success, graph_16.02)
	$(call test_success, graph_16.03)
	$(call test_success, graph_16.04)
	$(call test_success, graph_16.05)
	$(call test_success, graph_16.06)
	$(call test_success, graph_16.07)
	$(call test_success, graph_16.08)
	$(call test_success, graph_16.09)
	$(call test_success, graph_16.10)
	$(call test_failure, graph_17)
	$(call test_success, graph_18)
	$(call test_failure, graph_19)
	$(call test_failure, graph_20a)
	$(call test_failure, graph_20b)
	$(call test_failure, graph_21)
