BINARY := $(shell basename $(shell pwd))

SRC_DIR := src
OBJ_DIR := obj
DOC_DIR := doc

CPP := g++

CXXFLAGS += -DNCURSES_WIDECHAR -fpermissive
LDFLAGS += -lncursesw

CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
ifeq (,$(findstring noaftodo_embed.cpp,$(CPP_FILES)))
	CPP_FILES += src/noaftodo_embed.cpp
endif
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

all: embeds obj_dir $(OBJ_FILES) 
	@echo Source files: $(CPP_FILES)
	@echo Header files: $(H_FILES)
	@echo Object files: $(OBJ_FILES)
	@echo g++ flags: $(CXXFLAGS)
	@echo Linker flags: $(LDFLAGS)
	@echo Linking binary $(BINARY)...
	$(CPP) $(CXXFLAGS) -o $(BINARY) $(OBJ_FILES) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	@echo Compiling $@...
	$(CPP) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/noaftodo_embed.o: $(SRC_DIR)/noaftodo_embed.cpp
	@echo Compiling $@...
	$(CPP) $(CXXFLAGS) -c -o $@ $<

obj_dir:
	@-mkdir $(OBJ_DIR)

embeds:
	$(shell ./docgen.sh)

clean:
	@echo Removing object files...
	@-rm -rf $(OBJ_DIR)/*.o
	@-rm $(SRC_DIR)/noaftodo_embed.cpp
	@-rm $(DOC_DIR)/*doc.gen
	@-rmdir $(OBJ_DIR) # will fail if OBJ_DIR is not empty - as we want!
	@echo Removing execuatable
	@-rm $(BINARY)

install:
	install -Dm755 $(BINARY) $(PKGROOT)/usr/bin/$(BINARY)

uninstall:
	rm $(PKGROOT)/usr/bin/$(BINARY)
