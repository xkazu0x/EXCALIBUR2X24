CC := g++
CFLAGS := -std=c++17 -Wall -Wextra -g
INCLUDES := -I. -IC:\VulkanSDK\1.3.283.0\Include
LIBS := -luser32 -LC:\VulkanSDK\1.3.283.0\Lib -lvulkan-1
DEFINES := -DEXCALIBUR_DEBUG

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
all: shaders $(BUILD_DIR)/$(EXEC)

clean: cleanexe cleanspv

## EXECUTABLE
$(BUILD_DIR)/$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(DEFINES) $(INCLUDES)

$(BUILD_DIR) $(OBJ_DIR):
	$(MKDIR) $@

run:
	$(BUILD_DIR)/$(EXEC)

cleanexe:
	$(RMDIR) $(BUILD_DIR) $(OBJ_DIR)

shaders: $(SPIRV)

$(SHADER_DIR)/%.frag.spv: $(SHADER_DIR)/%.frag | $(SHADER_DIR)/%.vert.spv
	C:\VulkanSDK\1.3.283.0\Bin\glslc.exe $< -o $@

$(SHADER_DIR)/%.vert.spv: $(SHADER_DIR)/%.vert
	C:\VulkanSDK\1.3.283.0\Bin\glslc.exe $< -o $@

cleanspv:
	del res\shaders\*.spv
