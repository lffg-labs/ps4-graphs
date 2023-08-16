C := clang
CFLAGS := -Wall -Wextra -Wpedantic

TARGET := target
MAIN := $(TARGET)/main

$(MAIN): src/main.c $(TARGET)
	$(C) $(CFLAGS) -o $@ $<

$(TARGET):
	@mkdir $@

.PHONY: run
run: $(MAIN)
	@chmod +x $<
	./$<

.PHONY: clear
clean:
	rm -rf main target
