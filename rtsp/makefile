DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR


include $(DVD_MAKE)/generic.mak

# Exported libraries and headers
# ------------------------------
EXPORTS :=

# Resulting target
# ----------------
ST20_TARGETS := librtsp.a
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

SUBDIRS :=

# Objects in librtsp.lib
# ------------------------
ST20_OBJS := rtsp.tco           rtsp_comm.tco      rtsp_command.tco   rtsp_resp.tco      rtsp_util.tco \
sdp_decode.tco     sdp_dump.tco       sdp_util.tco

ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

include $(DVD_MAKE)/defrules.mak

# Rules to build component-specific targets
# =========================================


#----------------------------------------------------------------------
# include path for shared objects
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../..)
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../../../App/include)
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../../../Mid)
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../../../Mid/include)
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../../../../stapp/include)
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../../../../stapp)
INCLUDES += -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../../../Mid/TcpIP/include)

#For STTBX
ifneq "$(DVD_IO)" "NONE" 
$(OBJS):\
	CFLAGS := $(CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT -DSTTBX_INPUT
endif	

# Build TARGETS
# -------------
$(ST40_TARGETS) : $(ST40_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

display_variables:
	@$(ECHO) Display Build System variables
	@$(ECHO) SUBDIR_LIBS = $(SUBDIR_LIBS)
	@$(ECHO) SUBDIR_LIBRARY_PATH = $(SUBDIR_LIBRARY_PATH)
	@$(ECHO) INCLUDE_PATH = $(INCLUDE_PATH)

# Local header dependencies
# None


# Clean is mapped on clean_all to allow its propagation
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(ST40_TARGETS)
	-$(RM) $(ST40_OBJS)


else
include $(DVD_MAKE)/builddir.mak


endif
# EOF
