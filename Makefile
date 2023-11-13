RLC=/home/ccebeci/rlc/rlc-infrastructure/rlc-debug/tool/rlc/rlc -i /home/ccebeci/rlc/rlc-infrastructure/rlc/stdlib/
TRIS=/home/ccebeci/rlc/rlc-infrastructure/rlc/tool/rlc/test/tris.rl

all: main

main: include/tris.h obj/tris.o
	clang++ fuzz_target.cpp include/tris.h obj/tris.o -DRLC_ACTION=play
	
include/tris.h: $(TRIS)
	$(RLC) $(TRIS) --header -o include/tris.h

obj/tris.o: $(TRIS)
	$(RLC) $(TRIS) -o obj/tris.o -O2 --compile --emit-precondition-checks