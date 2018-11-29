SHELL:=/bin/bash
CPP=upcxx
CPP_MPI=mpic++
RUN=upcxx-run
RUN_MPI=mpirun
CPP_FLAGS=-O
DEBUG_FLAGS=-g -DDEBUG
RUN_FLAGS=-n 32
OUT_FILE=./bucket_sort_upcxx
OUT_FILE_MPI=./bucket_sort_mpi
BACKTRACE_FLAGS=-backtrace
LOAD_MODULE=module load gcc/5.1.0; 
DEBUG=1
ifeq ($(DEBUG), 1)
	CPP_FLAGS=$(DEBUG_FLAGS)
endif

.PHONY: run_bucket debug_bucket mpi_bucket clean

debug_bucket: bucket_sort_upcxx
	$(LOAD_MODULE) $(RUN) $(BACKTRACE_FLAGS) $(RUN_FLAGS) $(OUT_FILE)

run_bucket: bucket_sort_upcxx
	$(LOAD_MODULE) $(RUN) $(RUN_FLAGS) $(OUT_FILE)

mpi_bucket: bucket_sort_mpi
	$(RUN_MPI) $(RUN_FLAGS) $(OUT_FILE_MPI)

clean: 
	rm -f $(OUT_FILE) $(OUT_FILE_MPI)

%_upcxx: %_upcxx.cpp
	$(LOAD_MODULE) $(CPP) $(CPP_FLAGS) -o $@ $<

%_mpi: %_mpi.cpp
	$(CPP_MPI) $(CPP_FLAGS) -o $@ $<
