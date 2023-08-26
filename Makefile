CC := clang++
CCFLAGS := -std=c++2a -Wall -Wextra -Wpedantic
CCF := $(CC) $(CCFLAGS)

TARGET := target

$(shell mkdir -p $(TARGET))

$(TARGET)/representation: tasks/representation/main.cc
	$(CCF) -o $@ $^

.PHONY: clear
clean:
	rm -rf target
