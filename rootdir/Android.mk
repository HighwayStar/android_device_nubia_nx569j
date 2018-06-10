LOCAL_PATH:= $(call my-dir)

# Init scripts

include $(CLEAR_VARS)
LOCAL_MODULE       := init.nb.device.rc
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := init.nb.device.rc
LOCAL_MODULE_PATH  := $(TARGET_ROOT_OUT)
include $(BUILD_PREBUILT)

