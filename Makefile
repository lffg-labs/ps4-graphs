CC := clang++
CCFLAGS := -Wall -Wextra -Wpedantic

TARGET := target

.PHONY: clear
clean:
	rm -rf main target
