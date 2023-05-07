BDIR ?= build
CC ?= gcc
LDFLAGS ?=
CFLAGS ?= 
ifeq ($(RELEASE),)
	CFLAGS += -ggdb3 -fsanitize=address
	LDFLAGS += -lasan
	BDIR := $(BDIR)/debug
else
	CFLAGS += -O3
	BDIR := $(BDIR)/release
endif

# W celu dodania nowego pliku do kompilacji,
# trzeba dodac nazwe pliku objektowego do listy
OBJS := $(addprefix $(BDIR)/,main.o parser.o)

.PHONY: clean run dir build

run: build
	$(BDIR)/shell.out

build: $(BDIR)/shell.out ;

$(BDIR)/%.o: src/%.c
	$(CC) -Wall -Wextra $(CFLAGS) -c -o $@ $^

$(BDIR)/shell.out: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lreadline -lhistory 

$(OBJS): | $(BDIR)

$(BDIR):
	mkdir -p $(BDIR)

clean:
	rm -f $(OBJS) $(BDIR)/shell.out
