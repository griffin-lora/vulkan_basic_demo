TARGET := app

GLSLC := glslc

SOURCES := $(wildcard src/*.c) $(wildcard src/vk/*.c)
SHADER_SOURCES := $(wildcard shader/*.vert) $(wildcard shader/*.frag)
LIBS := -lvulkan -lglfw
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))
DEPENDS := $(patsubst %.c,%.d,$(SOURCES))
SHADER_OBJECTS := $(patsubst %.vert,%.spv,$(patsubst %.frag,%.spv,$(SHADER_SOURCES)))

CFLAGS = -O2 -Wall -Isrc

.PHONY: build run clean

build: $(OBJECTS) $(SHADER_OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET).elf $(OBJECTS) $(LIBS)

run: build
	@./$(TARGET).elf

clean:
	$(RM) $(OBJECTS) $(DEPENDS) $(SHADER_OBJECTS)

-include $(DEPENDS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.spv: %.vert Makefile
	$(GLSLC) $< -o $@

%.spv: %.frag Makefile
	$(GLSLC) $< -o $@