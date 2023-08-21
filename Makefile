CC := clang++
CCFLAGS := -std=c++2a -Wall -Wextra -Wpedantic

TARGET := target

$(TARGET)/representation: tasks/representation/main.cc $(TARGET)
	$(CC) $(CCFLAGS) -o $@ $<

$(TARGET):
	mkdir -p $@

.PHONY: clear
clean:
	rm -rf main target
