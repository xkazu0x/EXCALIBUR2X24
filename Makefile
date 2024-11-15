CC := g++
CFLAGS := -std=c++17 -Wall -Wextra -g
INCLUDES := -I. -IC:\VulkanSDK\1.3.283.0\Include
LIBS := -luser32 -LC:\VulkanSDK\1.3.283.0\Lib -lvulkan-1
DEFINES := -DEXCALIBUR_DEBUG

MKDIR := mkdir
RMDIR := rmdir /s /q

SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXEC := excalibur.exe

### ALL
all: $(BUILD_DIR)/$(EXEC)

clean: clnexec

## EXECUTABLE
$(BUILD_DIR)/$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(DEFINES) $(INCLUDES)

$(BUILD_DIR) $(OBJ_DIR):
	$(MKDIR) $@

run:
	$(BUILD_DIR)/$(EXEC)

clnexec:
	$(RMDIR) $(BUILD_DIR) $(OBJ_DIR)
