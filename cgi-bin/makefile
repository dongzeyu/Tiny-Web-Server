all:
	gcc adder.c -o adder
	gcc form-adder.c -o form-adder
	gcc head-adder.c -o head-adder
	gcc post-adder.c -o post-adder

.PHONY: clean
clean:
	find . -type f -executable -print0 | xargs -0 rm -f --
