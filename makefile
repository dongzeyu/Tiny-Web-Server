CFLAGS = -pthread -m64 -O2
SRCS = tiny.c wrapped.c

all:
	gcc $(CFLAGS) $(SRCS) -o tiny
	(cd cgi-bin; make)

	
.PHONY: clean
clean:
	find . -type f -executable -print0 | xargs -0 rm -f --
