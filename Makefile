# Copyright 2015 Gauthier Voron
# This file is part of rwmsr.
#
# Rwmsr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Rwmsr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with rwmsr. If not, see <http://www.gnu.org/licenses/>.

SCRIPT  := script/
DESTDIR ?= 

OBJ := obj/
LIB := lib/
BIN := bin/

SYSTEMS := $(shell ./$(SCRIPT)filter-systems.sh linux xen-tokyo)
TARGETS := $(BIN)rwmsr $(patsubst %, $(LIB)%.so, $(SYSTEMS))

CC        := gcc
CCFLAGS   := -Wall -Wextra -pedantic -O2 -Iinclude/
LDFLAGS   := -ldl -lrt
CCSOFLAGS := $(CCFLAGS)
LDSOFLAGS := 
CCXNFLAGS := -Wall -Wextra -O2 -Iinclude/ -Ixen-tokyo/
LDXNFLAGS := -lxenctrl

V ?= 1

ifeq ($(V),1)
  define print
    @echo "$(1)"
  endef
endif
ifneq ($(V),2)
  Q := @
endif


PHONY += default
default: all


PHONY += all
all: $(TARGETS)


$(BIN)rwmsr: $(OBJ)engine.o $(OBJ)loader.o $(OBJ)main.o $(OBJ)parse.o | $(BIN)
	$(call print,  LD      $@)
	$(Q)$(CC) -rdynamic $^ -o $@ $(LDFLAGS)

$(LIB)linux.so: $(OBJ)linux.so | $(LIB)
	$(call print,  LDSO    $@)
	$(Q)$(CC) -shared $^ -o $@ $(LDSOFLAGS)

$(LIB)xen-tokyo.so: $(OBJ)xen-tokyo.so | $(LIB)
	$(call print,  LDSO    $@)
	$(Q)$(CC) -shared $^ -o $@ $(LDXNFLAGS)


$(OBJ)%.o: common/%.c | $(OBJ)
	$(call print,  CC      $@)
	$(Q)$(CC) $(CCFLAGS) -c $< -o $@

$(OBJ)%.so: linux/%.c | $(OBJ)
	$(call print,  CCSO    $@)
	$(Q)$(CC) -fPIC $(CCSOFLAGS) -c $< -o $@

$(OBJ)%.so: xen-tokyo/%.c | $(OBJ)
	$(call print,  CCSO    $@)
	$(Q)$(CC) -fPIC $(CCXNFLAGS) -c $< -o $@


$(OBJ) $(LIB) $(BIN):
	$(call print,  MKDIR   $@)
	$(Q)mkdir $@


PHONY += install
install: all
	$(call print,  INSTALL $(DESTDIR)/)
	$(Q)./$(SCRIPT)install-dir.sh $(DESTDIR)/usr/lib/rwmsr
	$(Q)./$(SCRIPT)install-dir.sh $(DESTDIR)/usr/bin
	$(Q)cp $(LIB)* $(DESTDIR)/usr/lib/rwmsr
	$(Q)cp $(BIN)* $(DESTDIR)/usr/bin


PHONY += uninstall
uninstall:
	$(call print,  UNSTALL $(DESTDIR)/)
	$(Q)-rm $(patsubst $(LIB)%, $(DESTDIR)/usr/lib/rwmsr/%, \
	         $(filter $(LIB)%, $(TARGETS))) 2>/dev/null || true
	$(Q)-rm $(patsubst $(BIN)%, $(DESTDIR)/usr/bin/%, \
	         $(filter $(BIN)%, $(TARGETS))) 2>/dev/null || true
	$(Q)-./$(SCRIPT)uninstall-dir.sh $(DESTDIR)/usr/lib/rwmsr
	$(Q)-./$(SCRIPT)uninstall-dir.sh $(DESTDIR)/usr/bin


PHONY += clean
clean:
	$(call print,  CLEAN)
	-$(Q)rm -rf $(OBJ) $(LIB) $(BIN)


.PHONY: $(PHONY)
