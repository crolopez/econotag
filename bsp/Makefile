#
# Makefile del BSP para la Redwire EconoTAG
#

# Este makefile está escrito para una shell bash
SHELL = /bin/bash

#
# Paths y nombres de directorios
#

# Ruta al BSP
BSP_ROOT_DIR   = .

# Directorio para almacenar los ficheros objeto
OBJ_DIR        = $(BSP_ROOT_DIR)/obj

# Directorio para almacenar los ficheros de distribución del bsp
DIST_DIR        = $(BSP_ROOT_DIR)/dist

# Directorio de la toolchain de GNU
# Toolchain de los repositorios
TOOLS_PATH     = /usr
# Toolchain construida a partir de las fuentes en el ordenador local
#TOOLS_PATH     = /opt/econotag
# Toolchain construida a partir de las fuentes en los laboratorios
#TOOLS_PATH     = /fenix/depar/atc/se/toolchain

#
# Herramientas y cadena de desarrollo
#

# Herramientas del sistema
MKDIR          = mkdir -p
RM             = rm -rf

# Cadena de desarrollo
# Toolchain de los repositorios
TOOLS_PREFIX   = arm-none-eabi
# Toolchain construida a partir de las fuentes
#TOOLS_PREFIX   = arm-econotag-eabi

CROSS_COMPILE  = $(TOOLS_PATH)/bin/$(TOOLS_PREFIX)-
AS             = $(CROSS_COMPILE)as
CC             = $(CROSS_COMPILE)gcc
AR             = $(CROSS_COMPILE)ar

# Flags
ASFLAGS        = -gstabs -mcpu=arm7tdmi -mfpu=softfpa
CFLAGS         = -c -g -Wall -mcpu=arm7tdmi -std=gnu89
ARFLAGS        = -src

#
# Fuentes
#

# Todos los ficheros C del BSP
C_SRCS     = $(shell find $(BSP_ROOT_DIR) -name '*.c' -print)

# Todos los ficheros en ensamblador del BSP
ASM_SRCS   = $(shell find $(BSP_ROOT_DIR) -name '*.s' -print)

# Todos los ficheros cabecera del BSP
INCLUDES   = $(shell find $(BSP_ROOT_DIR) -name '*.h' -print)

#
# Lista de ficheros objeto
#

OBJS           = $(addprefix $(OBJ_DIR)/, $(ASM_SRCS:.s=.o) $(C_SRCS:.c=.o))

#
# Incluimos el Makefile público del BSP
#

include bsp.mk

ASFLAGS        += $(BSP_ASFLAGS)
CFLAGS         += $(BSP_CFLAGS)

#
# Reglas de construcción
#

.PHONY: all
all: $(BSP_LIB)

$(BSP_LIB): $(OBJS)
	@echo "Generando la biblioteca del bsp ..."
	$(AR) $(ARFLAGS) $@ $^
	@echo

$(OBJ_DIR)/%.o : %.s
	@echo "Ensamblando $< ..."
	@$(MKDIR) $(@D)
	$(AS) $(ASFLAGS) $< -o $@
	@echo

$(OBJ_DIR)/%.o : %.c
	@echo "Compilando $< ..."
	@$(MKDIR) $(@D)
	$(CC) $(CFLAGS) $< -o $@
	@echo

.PHONY: dist
dist: all
	@echo "Preparando el bsp para su distribución..."
	@$(MKDIR) $(DIST_DIR)
	@cp bsp.mk $(BSP_LIB) $(BSP_LINKER_SCRIPT) $(DIST_DIR)
	@$(MKDIR) $(DIST_DIR)/include
	@cp $(INCLUDES) $(DIST_DIR)/include

.PHONY: clean
clean:
	@echo "Limpiando el BSP ..."
	@$(RM) $(BSP_LIB) $(OBJ_DIR) $(DIST_DIR) $(shell find $(BSP_ROOT_DIR) -name '*~' -print)
