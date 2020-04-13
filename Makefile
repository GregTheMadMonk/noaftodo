BINARY := $(shell basename $(shell pwd))

SRC_DIR := src
OBJ_DIR := obj

CC := gcc
CXX := g++

CXX_FLAGS := 
CXX_LINKER_FLAGS := -lncursesw -lrt

CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
H_FILES := $(wildcard $(SRC_DIR)/*.h)

OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(patsubst %.cpp,%.o,$(CPP_FILES)))

all: obj_dir $(OBJ_FILES) noaftodo.conf.template
	@echo Source files: $(CPP_FILES)
	@echo Header files: $(H_FILES)
	@echo Object files: $(OBJ_FILES)
	@echo g++ flags: $(CXX_FLAGS)
	@echo Linker flags: $(CXX_LINKER_FLAGS)
	@echo Creting embed config...
	objcopy --input binary --output $(shell objdump -f $(OBJ_DIR)/noaftodo_config.o | grep file\ format | sed 's/.*file\ format.//g') --binary-architecture $(shell objdump -f $(OBJ_DIR)/noaftodo_config.o | grep architecture | sed 's/architecture:\ //g' | sed 's/,.*//g') noaftodo.conf.template $(OBJ_DIR)/noaftodo_config_template.o
	@echo Linking binary $(BINARY)...
	$(CXX) $(CXX_FLAGS) -o $(BINARY) $(OBJ_FILES) $(OBJ_DIR)/noaftodo_config_template.o $(CXX_LINKER_FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@echo Compiling $@...
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

obj_dir:
	@-mkdir $(OBJ_DIR)
clean:
	@echo Removing object files...
	@-rm -rf $(OBJ_DIR)/*.o
	@-rmdir $(OBJ_DIR) # will fail if OBJ_DIR is not empty - as we want!
	@echo Removing execuatable
	@-rm $(BINARY)
