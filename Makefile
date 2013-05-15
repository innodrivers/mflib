-include local.mk

# Find the local dir of the make file
GET_LOCAL_DIR    = $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi


include $(ECOS_KERNEL)/include/pkgconf/ecos.mak
XCC           = $(ECOS_COMMAND_PREFIX)gcc
XCXX          = $(ECOS_COMMAND_PREFIX)g++
XLD           = $(ECOS_COMMAND_PREFIX)gcc
XAR           = $(ECOS_COMMAND_PREFIX)ar
OBJCOPY	      =	$(ECOS_COMMAND_PREFIX)objcopy


# default lib and app name, can override them
MIFI_LIB ?= libmf
MIFI_APP ?= mf_main

BUILDDIR := build-$(PROJECT)-$(BOARD)
CONFIGHEADER := $(BUILDDIR)/config.h


BUILDTIME := $(shell date "+%Y.%m.%d-%H:%M:%S")

INCLUDES := -I$(BUILDDIR) -Iinclude

DEFINES := PROJECT_$(PROJECT)=1 BOARD_$(BOARD)=1 LOG_OUT_$(LOG_OUT)=1

LDFLAGS += -L$(ECOS_KERNEL)/lib
LDFLAGS += -Ttarget.ld 
LDFLAGS += -ltarget
CFLAGS += -I$(ECOS_KERNEL)/include -include $(CONFIGHEADER)


SRCDEPS	:= $(CONFIGHEADER)

OBJS :=

include src/rules.mk
include board/$(BOARD)/rules.mk
include project/$(PROJECT)/rules.mk

TARGETLIB := $(BUILDDIR)/$(MIFI_LIB).a
TARGETBIN := $(BUILDDIR)/$(MIFI_APP).bin
TARGETELF := $(BUILDDIR)/$(MIFI_APP).elf

ALLOBJS :=	\
	$(OBJS)

ALLOBJS := $(addprefix $(BUILDDIR)/, $(ALLOBJS))
DEPS := $(ALLOBJS:%o=%d)

$(BUILDDIR)/%.o : %.c $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	$(XCC) $(CFLAGS) $(ECOS_GLOBAL_CFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@

$(BUILDDIR)/%.o : %.S $(SRCDEPS)
	@$(MKDIR)
	@echo compiling $<
	@$(XCC) $(CFLAGS) $(ASMFLAGS) $(ECOS_GLOBAL_CFLAGS) $(INCLUDES) -c $< -MD -MT $@ -MF $(@:%o=%d) -o $@


$(TARGETBIN) : $(TARGETELF)
	$(OBJCOPY) -O binary $< $@

$(TARGETELF) : $(TARGETLIB)
	$(XLD) $(ECOS_GLOBAL_LDFLAGS) -o $@ $(ALLOBJS) $(LDFLAGS)
	

$(TARGETLIB): $(ALLOBJS)
	$(XAR) rcs $@ $(ALLOBJS)

configheader:

$(CONFIGHEADER): configheader
	@$(MKDIR)
	@echo generating $@
	@rm -f $(CONFIGHEADER).tmp; \
	echo \#ifndef __CONFIG_H > $(CONFIGHEADER).tmp; \
	echo \#define __CONFIG_H >> $(CONFIGHEADER).tmp; \
	for d in `echo $(DEFINES) | tr [:lower:] [:upper:]`; do \
		echo "#define $$d" | sed "s/=/\ /g;s/-/_/g;s/\//_/g" >> $(CONFIGHEADER).tmp; \
	done; \
	echo \#endif >> $(CONFIGHEADER).tmp; \
	if [ -f "$(CONFIGHEADER)" ]; then \
		if cmp "$(CONFIGHEADER).tmp" "$(CONFIGHEADER)"; then \
			rm -f $(CONFIGHEADER).tmp; \
		else \
			mv $(CONFIGHEADER).tmp $(CONFIGHEADER); \
		fi \
	else \
		mv $(CONFIGHEADER).tmp $(CONFIGHEADER); \
	fi

%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif

all: $(TARGETLIB) $(TARGETBIN)


clean:
	@echo Cleaning...
	-rm -f $(ALLOBJS) 
	-rm -f $(TARGETLIB)
	-rm -f $(TARGETBIN) $(TARGETELF)

