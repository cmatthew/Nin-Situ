CC = nacl64-gcc
CFLAGS = -c -std=gnu99 -std=gnu99 -Wall -Wextra -Wno-long-long -O2 -fomit-frame-pointer -Wall -fdiagnostics-show-option -pedantic -Werror -m64 -DLINUX
INCLUDE = -I /home/lind/tmp/lind/sdk/linux_x86/nacl64/include/
LDFLAGS = -melf64_nacl -m64

EXECUTABLE = nin-situ

ifeq ($(type),c)
	CC = gcc
	CFLAGS = -g -c -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wno-long-long -O0 -fomit-frame-pointer -m64 -DLINUX
	LDFLAGS = -m64
	EXECUTABLE = nin-situ
endif

SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

all: tags $(SOURCES) $(EXECUTABLE)
	@echo
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

tags:
	etags *.c

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)

check-syntax:
	gcc $(CFLAGS) -o nul -S ${CHK_SOURCES}

valgrind:
ifeq ($(type),c)

	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 ./nin-situ	
endif