TARGET := app

GLSLC := glslc

SHADER_SOURCES := $(wildcard shader/*.vert) $(wildcard shader/*.frag) $(wildcard shader/*.comp) $(wildcard shader/*.mesh) $(wildcard shader/*.task)
SHADER_OBJECTS := $(patsubst %.vert,%.spv,$(patsubst %.frag,%.spv,$(patsubst %.comp,%.spv,$(patsubst %.mesh,%.spv,$(patsubst %.task,%.spv,$(SHADER_SOURCES))))))

GLSLFLAGS = --target-env=vulkan1.3

.PHONY: build run clean

all: $(SHADER_OBJECTS)

%.spv: %.vert Shaders.mk
	$(GLSLC) $(GLSLFLAGS) $< -o $@

%.spv: %.frag Shaders.mk
	$(GLSLC) $(GLSLFLAGS) $< -o $@

%.spv: %.comp Shaders.mk
	$(GLSLC) $(GLSLFLAGS) $< -o $@

%.spv: %.mesh Shaders.mk
	$(GLSLC) $(GLSLFLAGS) $< -o $@

%.spv: %.task Shaders.mk
	$(GLSLC) $(GLSLFLAGS) $< -o $@