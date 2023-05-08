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
OBJS := $(addprefix $(BDIR)/,main.o parser.o vecstring.o)
DEPS := $(OBJS:.o=.d)

.PHONY: clean run build

run: build
	$(BDIR)/shell.out

build: $(BDIR)/shell.out ;

-include $(DEPS)

$(BDIR)/%.o: src/%.c
	$(CC) -Wall -Wextra $(CFLAGS) -Isrc \
		-MMD -c -o $@ $<

$(BDIR)/shell.out: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lreadline -lhistory 

$(OBJS): | $(BDIR)

$(BDIR):
	mkdir -p $(BDIR)

clean:
	rm -f $(OBJS) $(DEPS) $(BDIR)/shell.out
