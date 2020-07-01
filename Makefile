BINARY := $(shell basename $(shell pwd))

SRC_DIR := src
OBJ_DIR := obj
DOC_DIR := doc

CPP := g++

CXXFLAGS += -DNCURSES_WIDECHAR -fpermissive
LDFLAGS += -lncursesw

CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
H_FILES := $(wildcard $(SRC_DIR)/*.h)

OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(patsubst %.cpp,%.o,$(CPP_FILES)))

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),SunOS)
	OBJCOPY := "/usr/gnu/bin/objcopy"
	OBJDUMP := "/usr/gnu/bin/objdump"
else
	OBJCOPY := "/bin/objcopy"
	OBJDUMP := "/bin/objdump"
endif

ifeq ($(UNAME_S),Haiku) 
	# couldn't find mqueue.h on Haiku
	NO_MQUEUE := 1
	# Haiku is single-user (isn't it?)
	NO_ROOT_CHECK := 1
endif

ifeq ($(NO_MQUEUE), 1)
	CXXFLAGS += -DNO_MQUEUE
else
	LDFLAGS += -lrt
endif

ifeq ($(NO_ROOT_CHECK), 1)
	CXXFLAGS += -DNO_ROOT_CHECK
endif

ifeq ($(DEBUG), 1)
	CXXFLAGS += -g3
endif

all: obj_dir $(OBJ_FILES) noaftodo.conf.template doc
	@echo Source files: $(CPP_FILES)
	@echo Header files: $(H_FILES)
	@echo Object files: $(OBJ_FILES)
	@echo g++ flags: $(CXXFLAGS)
	@echo Linker flags: $(LDFLAGS)
	@echo Creting embed config...
	$(OBJCOPY) --input-target binary --output-target $(shell $(OBJDUMP) -f $(OBJ_DIR)/noaftodo_config.o | grep file\ format | sed 's/.*file\ format.//g') --binary-architecture $(shell $(OBJDUMP) -f $(OBJ_DIR)/noaftodo_config.o | grep architecture | sed 's/architecture:\ //g' | sed 's/,.*//g') noaftodo.conf.template $(OBJ_DIR)/noaftodo_config_template.o
	@echo Creating embed help...
	$(OBJCOPY) --input-target binary --output-target $(shell $(OBJDUMP) -f $(OBJ_DIR)/noaftodo_config.o | grep file\ format | sed 's/.*file\ format.//g') --binary-architecture $(shell $(OBJDUMP) -f $(OBJ_DIR)/noaftodo_config.o | grep architecture | sed 's/architecture:\ //g' | sed 's/,.*//g') $(DOC_DIR)/doc.gen $(OBJ_DIR)/noaftodo_doc.o
	@echo Linking binary $(BINARY)...
	$(CPP) $(CXXFLAGS) -o $(BINARY) $(OBJ_FILES) $(OBJ_DIR)/noaftodo_config_template.o $(OBJ_DIR)/noaftodo_doc.o $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@echo Compiling $@...
	$(CPP) $(CXXFLAGS) -c -o $@ $<

obj_dir:
	@-mkdir $(OBJ_DIR)

doc_dir:
	@-mkdir $(DOC_DIR)

doc: doc_dir $(SRC_DIR)/noaftodo_cmd.cpp
	$(shell ./docgen.sh)
clean:
	@echo Removing object files...
	@-rm -rf $(OBJ_DIR)/*.o
	@-rm $(DOC_DIR)/*doc.gen
	@-rmdir $(OBJ_DIR) # will fail if OBJ_DIR is not empty - as we want!
	@echo Removing execuatable
	@-rm $(BINARY)

install:
	install -Dm755 $(BINARY) $(PKGROOT)/usr/bin/$(BINARY)

uninstall:
	rm $(PKGROOT)/usr/bin/$(BINARY)
