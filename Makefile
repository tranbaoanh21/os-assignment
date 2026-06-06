
INC = -Iinclude
LIB = -lpthread

SRC = src
OBJ = obj
INCLUDE = include

CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

vpath %.c $(SRC)
vpath %.h $(INCLUDE)

MAKE = $(CC) $(INC) 

# Object files needed by modules
MEM_OBJ = $(addprefix $(OBJ)/, paging.o mem.o cpu.o loader.o)
SYSCALL_OBJ = $(addprefix $(OBJ)/, syscall.o  sys_mem.o sys_listsyscall.o sys_xxxhandler.o)
OS_OBJ = $(addprefix $(OBJ)/, cpu.o mem.o loader.o queue.o os.o sched.o timer.o mm-vm.o mm64.o mm.o mm-memphy.o libstd.o libmem.o)
OS_OBJ += $(SYSCALL_OBJ)
HEADER = $(wildcard $(INCLUDE)/*.h)
 
all: os
#mem sched os

report:
	TEXMFVAR=/tmp/ossim-texmf-var TEXMFCONFIG=/tmp/ossim-texmf-config XDG_CACHE_HOME=/tmp/ossim-cache \
	latexmk -lualatex -interaction=nonstopmode -halt-on-error assignment_os_report.tex

# Compatibility binaries use the complete simulator because CPU instructions
# now enter memory management through the unified syscall layer.
mem: os
	cp os mem

sched: os
	cp os sched

test: os
	sh ./run.sh

# Compile syscall
$(SRC)/syscalltbl.lst: $(SRC)/syscall.tbl
	@echo $(OS_OBJ)
	chmod +x $(SRC)/syscalltbl.sh
	$(SRC)/syscalltbl.sh $< $@
#	mv $(OBJ)/syscalltbl.lst $(INCLUDE)/

# Compile the whole OS simulation
os: $(OBJ) $(SRC)/syscalltbl.lst $(OS_OBJ)
	$(MAKE) $(LFLAGS) $(OS_OBJ) -o os $(LIB)

$(OBJ)/%.o: %.c ${HEADER} | $(OBJ)
	$(MAKE) $(CFLAGS) $< -o $@

$(OBJ)/syscall.o: $(SRC)/syscalltbl.lst

# Prepare objectives container
$(OBJ):
	mkdir -p $(OBJ)

clean:
	rm -f $(SRC)/*.lst
	rm -f $(OBJ)/*.o os sched mem pdg
	rm -rf $(OBJ)
