include Makefile.defines.in

CCFLAGS += -std=c++11

OBJFILES = $(OBJ_DIR)/WriteSet.o 
OBJFILES += $(OBJ_DIR)/LightHW.o $(OBJ_DIR)/BFHW.o
OBJFILES += $(OBJ_DIR)/SpecSW.o $(OBJ_DIR)/SglSW.o $(OBJ_DIR)/IrrevocSW.o

.PHONY: clean

all:  $(OBJ_DIR)/test_threads 

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/test_threads: $(OBJFILES)
	$(CPP) $(CCFLAGS) -o $@ $(OBJFILES) $(LDFLAGS)
	cp $(OBJ_DIR)/test_threads .


$(OBJ_DIR)/test_t.o: $(OBJ_DIR) $(SRC_DIR)/test_threads.cpp 
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test_threads.cpp -c -o $@

$(OBJ_DIR)/WriteSet.o: $(OBJ_DIR) $(SRC_DIR)/tm/WriteSet.c $(SRC_DIR)/tm/WriteSet.hpp
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/tm/WriteSet.c -c -o $@

$(OBJ_DIR)/LightHW.o: $(OBJ_DIR) $(SRC_DIR)/tm/LightHW.cpp
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/tm/LightHW.c -c -o $@

$(OBJ_DIR)/BFHW.o: $(OBJ_DIR) $(SRC_DIR)/tm/BFHW.cpp
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/tm/BFHW.c -c -o $@

$(OBJ_DIR)/SpecSW.o: $(OBJ_DIR) $(SRC_DIR)/tm/SpecSW.cpp
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/tm/SpecSW.c -c -o $@

$(OBJ_DIR)/SglSW.o: $(OBJ_DIR) $(SRC_DIR)/tm/SglSW.cpp
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/tm/SglSW.c -c -o $@

$(OBJ_DIR)/IrrevocSW.o: $(OBJ_DIR) $(SRC_DIR)/tm/IrrevocSW.cpp
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/tm/IrrevocSW.c -c -o $@


################
# common tasks #
################

clean:
	rm -rf $(TARGET_DIR)
	rm -f test_threads


