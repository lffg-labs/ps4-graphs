CC := clang++
CCFLAGS := -std=c++2a -Wall -Wextra -Wpedantic

RELEASE := 0
SANITY_CHECK := 0

ifeq ($(RELEASE),0)
TARGET := target/debug
else
TARGET := target/release
CCFLAGS += -O3
endif

ifneq ($(SANITY_CHECK),0)
CCFLAGS += -DSANITY_CHECK
endif

CCF := $(CC) $(CCFLAGS)

$(shell mkdir -p $(TARGET))

SRC := tasks
SRCS := $(patsubst $(SRC)/%,%,$(wildcard $(SRC)/*))

# default target: build all
.PHONY: all
all: $(patsubst %,$(TARGET)/%,$(SRCS))

# maybe-todo: https://stackoverflow.com/a/11441134
$(TARGET)/representation-star: tasks/representation-star/main.cc
$(TARGET)/depth-search: tasks/depth-search/main.cc
	$(CCF) -o $@ $^

# build-% utility
BUILD_TARGETS := $(patsubst %,build-%,$(SRCS))
.PHONY: $(BUILD_TARGETS)
$(BUILD_TARGETS): build-%: $(TARGET)/%

# run-% utility
RUN_TARGETS := $(patsubst %,run-%,$(SRCS))
.PHONY: $(RUN_TARGETS)
$(RUN_TARGETS): run-%: $(TARGET)/%
	./$< $(ARGS)

.PHONY: clear
clean:
	rm -rf target
