CC := g++
CFLAGS := -std=c++17 -Wall -Wextra -g
INCLUDES := -I. -Ivendor\includes -IC:\VulkanSDK\1.3.283.0\Include
LIBS := -luser32 -lwinmm -LC:\VulkanSDK\1.3.283.0\Lib -lvulkan-1
DEFINES := -DEXCALIBUR_DEBUG

GLSLC := C:\VulkanSDK\1.3.283.0\Bin\glslc.exe

MKDIR := mkdir
RMDIR := rmdir /s /q

RES_DIR := res
SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXEC := excalibur.exe

SHADER_DIR := $(RES_DIR)/shaders
SHADERS := $(wildcard $(SHADER_DIR)/*.vert) $(wildcard $(SHADER_DIR)/*.frag)
SPIRV := $(SHADERS:%.vert=%.vert.spv) $(SHADERS:%.frag=%.frag.spv)

### ALL
all: shader $(BUILD_DIR)/$(EXEC)

clean: cleanexe cleanshader

## EXECUTABLE
$(BUILD_DIR)/$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(DEFINES) $(INCLUDES)

$(BUILD_DIR) $(OBJ_DIR):
	$(MKDIR) $@

run: all
	$(BUILD_DIR)/$(EXEC)

cleanexe:
	$(RMDIR) $(BUILD_DIR) $(OBJ_DIR)

## SHADER
shader: $(SPIRV)

$(SHADER_DIR)/%.spv: $(SHADER_DIR)/%
	$(GLSLC) $< -o $@

cleanshader:
	del res\shaders\*.spv
