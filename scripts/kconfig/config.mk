LOCAL_DIR := scripts/kconfig

EXTRA_CFLAGS := $(HOST_CFLAGS)
EXTRA_CFLAGS += -I$(LOCAL_DIR)

EXTRA_LDLIBS := $(HOST_LDLIBS)

.PHONY: config oldconfig silentoldconfig menuconfig savedefconfig defconfig
config: $(LOCAL_DIR)/conf
	$< --oldaskconfig $(Kconfig)

oldconfig: $(LOCAL_DIR)/conf
	$< --$@ $(Kconfig)

silentoldconfig: $(LOCAL_DIR)/conf
	@$(MKDIR) include/config include/generated
	$< --$@ $(Kconfig)

menuconfig: $(LOCAL_DIR)/mconf
	$< $(Kconfig)

savedefconfig: $(LOCAL_DIR)/conf
	$< --$@=defconfig $(Kconfig)

defconfig: $(LOCAL_DIR)/conf
	@echo "*** Default configuration is based on 'deconfig'"
	@$< --defconfig=src/configs/defconfig $(Kconfig)

%_defconfig: $(LOCAL_DIR)/conf
	@$< --defconfig=src/configs/$@ $(Kconfig)

.PHONY: help
help:
	@echo  '  config          - Update current config utilising a line-oriented program'
	@echo  '  menuconfig      - Update current config utilising a menu based program'
	@echo  '  oldconfig       - Update current config utilising a provided .config as base'
	@echo  '  silentoldconfig - Same as oldconfig, but quietly, additionally update deps'
	@echo  '  defconfig       - New config with default from ARCH supplied defconfig'
	@echo  '  savedefconfig   - Save current config as ./defconfig (minimal config)'

# Sources
lxdialog := $(LOCAL_DIR)/lxdialog/checklist.c
lxdialog += $(LOCAL_DIR)/lxdialog/util.c
lxdialog += $(LOCAL_DIR)/lxdialog/inputbox.c
lxdialog += $(LOCAL_DIR)/lxdialog/textbox.c
lxdialog += $(LOCAL_DIR)/lxdialog/yesno.c
lxdialog += $(LOCAL_DIR)/lxdialog/menubox.c

mconfsrc := $(LOCAL_DIR)/mconf.c
mconfsrc += $(LOCAL_DIR)/zconf.tab.c

confsrc := $(LOCAL_DIR)/conf.c
confsrc += $(LOCAL_DIR)/zconf.tab.c

# Check ncurses
check_lxdialog := $(LOCAL_DIR)/lxdialog/check-lxdialog.sh
EXTRA_CFLAGS += $(shell $(HOST_SHELL) $(check_lxdialog) -ccflags) -DLOCALE
EXTRA_LDLIBS += $(shell $(HOST_SHELL) $(check_lxdialog) -ldflags $(HOSTCC))

.PHONY: dochecklxdialog
dochecklxdialog:
	@$(HOST_SHELL) $(check_lxdialog) -check	\
		$(HOSTCC) $(EXTRA_CFLAGS) $(EXTRA_LDLIBS)
$(lxdialog): dochecklxdialog

quiet_msg_hostcc = "  HOSTCC  "

$(LOCAL_DIR)/mconf: $(mconfsrc) $(lxdialog)
	$(Q)$(HOSTCC) $(EXTRA_CFLAGS) -o $@ $^ $(EXTRA_LDLIBS)
	@$(call show_msg,hostcc,$@)
	@$(HOST_STRIP) $@

$(LOCAL_DIR)/conf: $(confsrc)
	$(Q)$(HOSTCC) $(HOST_CFLAGS) -o $@ $^ $(HOST_LDLIBS)
	@$(call show_msg,hostcc,$@)
	@$(HOST_STRIP) $@

# Files to clean
DISTCLEAN_FILES += $(LOCAL_DIR)/conf
DISTCLEAN_FILES += $(LOCAL_DIR)/mconf
