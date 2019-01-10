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
LOAD_MODULE_UPCXX=module unload gcc/4.9.3; module load gcc/5.1.0; 
LOAD_MODULE_MPI=module unload gcc/5.1.0; module load gcc/4.9.3; 
DEBUG=1
ifeq ($(DEBUG), 1)
	CPP_FLAGS=$(DEBUG_FLAGS)
endif
CPP_FLAGS += -std=c++11 

debug_upcxx: bucket_sort_upcxx
	$(LOAD_MODULE_UPCXX) $(RUN) $(BACKTRACE_FLAGS) $(RUN_FLAGS) $(OUT_FILE)

run_upcxx: bucket_sort_upcxx
	$(LOAD_MODULE_UPCXX) $(RUN) $(RUN_FLAGS) $(OUT_FILE)

run_mpi: bucket_sort_mpi
	$(LOAD_MODULE_MPI) $(RUN_MPI) $(RUN_FLAGS) $(OUT_FILE_MPI)

clean: 
	rm -f $(OUT_FILE) $(OUT_FILE_MPI)

%_upcxx: %_upcxx.cpp
	$(LOAD_MODULE_UPCXX) $(CPP) $(CPP_FLAGS) -o $@ $<

%_mpi: %_mpi.cpp
	$(LOAD_MODULE_MPI) $(CPP_MPI) $(CPP_FLAGS) -o $@ $<

.PHONY: run_bucket debug_bucket mpi_bucket clean
