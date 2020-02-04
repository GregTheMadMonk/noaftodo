BINARY := $(shell basename $(shell pwd))

SRC_DIR := src/
OBJ_DIR := obj/

CC := gcc
CCX := g++

CXX_FLAGS := -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/lib/libffi-3.2.1/include -I/usr/include/libmount -I/usr/include/blkid
CXX_LINKER_FLAGS := -pthread -lnotify -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0  -lncursesw -lrt

CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
H_FILES := $(wildcard $(SRC_DIR)/*.h)

OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(patsubst %.cpp,%.o,$(CPP_FILES)))

all: obj_dir $(OBJ_FILES)
	@echo Source files: $(CPP_FILES)
	@echo Header files: $(H_FILES)
	@echo Object files: $(OBJ_FILES)
	@echo g++ flags: $(CXX_FLAGS)
	@echo Linker flags: $(CXX_LINKER_FLAGS)
	@echo Linking binary $(BINARY)...
	$(CXX) $(CXX_FLAGS) -o $(BINARY) $(OBJ_FILES) $(CXX_LINKER_FLAGS)

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
