CC = nacl64-gcc
CFLAGS = -c -std=gnu99 -std=gnu99 -Wall -Wextra -Wno-long-long -O2 -fomit-frame-pointer -Wall -fdiagnostics-show-option -pedantic -Werror -m64 -DLINUX
INCLUDE = -I /home/lind/tmp/lind/sdk/linux_x86/nacl64/include/
LDFLAGS = -melf64_nacl -m64

EXECUTABLE = nin-situ


ifeq ($(type),c)
	CC = gcc
	CFLAGS = -g -c -std=gnu99 -std=gnu99 -Wall -Wno-long-long -O2 -fomit-frame-pointer -fdiagnostics-show-option -m64 -DLINUX
	LDFLAGS = -m64
	EXECUTABLE = nin-situ

endif

SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE);
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
