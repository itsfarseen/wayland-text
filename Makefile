builddir	:= build
CFLAGS		:=
LDFLAGS		:= -lwayland-client

run: $(builddir)/main
	$(builddir)/main

clean:
	rm -rf build

$(builddir)/main: main.c
	mkdir -p $(builddir)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)
