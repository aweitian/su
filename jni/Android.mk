LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall
LOCAL_SRC_FILES := su.c checkperm.c setlogin.c log.c
LOCAL_MODULE := su
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
# LOCAL_STATIC_LIBRARIES := libc
# LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)
