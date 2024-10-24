NAME = nemu

ifneq ($(MAKECMDGOALS),clean) # ignore check for make clean
ISA ?= x86
ISAS = $(shell ls src/isa/)
ifeq ($(filter $(ISAS), $(ISA)), ) # ISA must be valid
$(error Invalid ISA. Supported: $(ISAS))
endif

ENGINE ?= interpreter
ENGINES = $(shell ls src/engine/)
ifeq ($(filter $(ENGINES), $(ENGINE)), ) # ENGINE must be valid
$(error Invalid ENGINE. Supported: $(ENGINES))
endif

$(info Building $(ISA)-$(NAME)-$(ENGINE))

endif

INC_DIR += ./include ./src/engine/$(ENGINE) ./src/isa/riscv64/softfloat ./resource
BUILD_DIR ?= ./build

ifdef SHARE
SO = -so
SO_CFLAGS = -fPIC -D_SHARE=1
SO_LDLAGS = -shared -fPIC
endif

ifndef SHARE
DIFF ?= kvm
ifneq ($(ISA),x86)
ifeq ($(DIFF),kvm)
DIFF = qemu
$(info KVM is only supported with ISA=x86, use QEMU instead)
endif
endif

ifeq ($(DIFF),qemu)
DIFF_REF_PATH = $(NEMU_HOME)/tools/qemu-diff
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-qemu-so
CFLAGS += -D__DIFF_REF_QEMU__
else ifeq ($(DIFF),kvm)
DIFF_REF_PATH = $(NEMU_HOME)/tools/kvm-diff
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-kvm-so
CFLAGS += -D__DIFF_REF_KVM__
else ifeq ($(DIFF),nemu)
DIFF_REF_PATH = $(NEMU_HOME)
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-nemu-interpreter-so
CFLAGS += -D__DIFF_REF_NEMU__
MKFLAGS = ISA=$(ISA) SHARE=1 ENGINE=interpreter
else
$(error invalid DIFF. Supported: qemu kvm nemu)
endif
endif

OBJ_DIR ?= $(BUILD_DIR)/obj-$(ISA)-$(ENGINE)$(SO)
BINARY ?= $(BUILD_DIR)/$(ISA)-$(NAME)-$(ENGINE)$(SO)

include Makefile.git

.DEFAULT_GOAL = app

ifdef XIANGSHAN
	CFLAGS += -DXIANGSHAN=1
endif

# Compilation flags
CC = g++
LD = g++
INCLUDES  = $(addprefix -I, $(INC_DIR))
CFLAGS   += -O2 -MMD -Wno-format -Wall \
			-std=c++17 \
			-Werror \
			-ggdb3 $(INCLUDES) \
            -D__ENGINE_$(ENGINE)__ \
            -D__SIMPOINT \
	    -DFLAT_CPTPATH \
            -D__GCPT_COMPATIBLE__ \
            -D__ISA__=$(ISA) -D__ISA_$(ISA)__ -D_ISA_H_=\"isa/$(ISA).h\"
			# -Wc++-compat \

# Files to be compiled
SRCS = $(shell find src/ -name "*.c" | grep -v "isa\|engine")
CPP_SRCS += $(shell find src/ -name "*.cpp" | grep -v "isa\|engine")
SRCS += $(shell find src/isa/$(ISA) -name "*.c")
SRCS += $(shell find src/engine/$(ENGINE) -name "*.c")
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
CPP_OBJS += $(CPP_SRCS:src/%.cpp=$(OBJ_DIR)/%.o)
OBJS += $(CPP_OBJS)

# Compilation patterns

$(OBJ_DIR)/isa/riscv64/softfloat/%.o: src/isa/riscv64/softfloat/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -D__cplusplus -w -fPIC -c -o $@ $<

$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/%.cpp
	@echo + CC $<
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<


# Depencies
-include $(OBJS:.o=.d)

# Some convenient rules

.PHONY: app run gdb clean run-env $(DIFF_REF_SO) profile checkpointing
app: $(BINARY)

override ARGS ?= --log=$(BUILD_DIR)/nemu-log.txt
override ARGS += --diff=$(DIFF_REF_SO)

# Command to execute NEMU
IMG :=
NEMU_EXEC := $(BINARY) $(ARGS) $(IMG)

$(BINARY): $(OBJS)
	$(call git_commit, "compile")
	@echo + LD $@
	@$(LD) -O2 -rdynamic $(SO_LDLAGS) -o $@ $^ -lSDL2 -lreadline -ldl -lz
# @echo + LD inputs $^

run-env: $(BINARY) $(DIFF_REF_SO)

run: run-env
	$(call git_commit, "run")
	$(NEMU_EXEC)

gdb: run-env
	$(call git_commit, "gdb")
	gdb -s $(BINARY) --args $(NEMU_EXEC)

$(DIFF_REF_SO):
	$(MAKE) -C $(DIFF_REF_PATH) $(MKFLAGS)

clean:
	-rm -rf $(BUILD_DIR)
	$(MAKE) -C tools/gen-expr clean
	$(MAKE) -C tools/qemu-diff clean
	$(MAKE) -C tools/kvm-diff clean

CLUSTER_DIR = /home/lizilin/projects/xs_gem5/checkpoints/cluster
BENCH_NAME = perlbench_r

build_environment:
	cd riscv-rootfs/rootfsimg && python spec_gen.py ${BENCH_NAME} --checkpoints && cd ../..
	make -C riscv-linux ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu-
	make -C riscv-pk clean
	make -C riscv-pk -j8
	mkdir -p ../checkpoints/${BENCH_NAME}
	mkdir -p ../checkpoints/cluster/${BENCH_NAME}
	mv riscv-pk/build/bbl.bin riscv-pk/bin/${BENCH_NAME}.bin


profile: build_environment
	./build/riscv64-nemu-interpreter riscv-pk/bin/${BENCH_NAME}.bin -D ../checkpoints/${BENCH_NAME} -w ${BENCH_NAME} -C profiling -b --simpoint-profile --interval 100000000

cluster: profile
	./resource/simpoint/simpoint_repo/bin/simpoint -loadFVFile /home/lizilin/projects/xs_gem5/checkpoints/${BENCH_NAME}/profiling/simpoint_bbv.gz -saveSimpoints ${CLUSTER_DIR}/${BENCH_NAME}/simpoints0 -saveSimpointWeights ${CLUSTER_DIR}/${BENCH_NAME}/weights0 -inputVectorsGzipped -maxK 30 -numInitSeeds 2 -iters 100 -seedkm 123456 -seedproj 654321

checkpointing: cluster
	./build/riscv64-nemu-interpreter riscv-pk/bin/${BENCH_NAME}.bin -D ../checkpoints/${BENCH_NAME} -w ${BENCH_NAME} -C take_cpt -b -S ${CLUSTER_DIR} --checkpoint-interval 100000000
