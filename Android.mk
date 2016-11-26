LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)


LOCAL_LDLIBS:= -L$(SYSROOT)/usr/lib -llog

LOCAL_SRC_FILES:=  \
	main.c console.c ui.c font_CN16x32.c event.c \
	board_test.c process.c event_queue.c serial.c \
	virt_serial.c

LOCAL_C_INCLUDES := \
	config.h  ui.h event.h board_test.h  process.h 

LOCAL_C_INCLUDES += system/core/include/cutils
LOCAL_C_INCLUDES += external/tinyalsa/include

LOCAL_CFLAGS := -D_POSIX_SOURCE -DLINUX -w
LOCAL_STATIC_LIBRARIES := \
    libext4_utils_static \
    libsparse_static \
    libminzip \
    libz \
    libmtdutils \
    libmincrypt \
    libminadbd \
    libfusesideload \
    libminui \
    libpng \
    libfs_mgr \
    libbase \
    libcutils \
    liblog \
    libselinux \
    libstdc++ \
    libm \
    libc \
    libcutils \
    libutils \
    libtinyalsa



LOCAL_MODULE_TAGS := eng
LOCAL_MULTILIB := 64
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/sbin
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE:= engmode
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES:=  \
	test.c

LOCAL_C_INCLUDES := \
	config.h  ui.h event.h board_test.h  process.h 

LOCAL_CFLAGS := -D_POSIX_SOURCE -DLINUX
LOCAL_SHARED_LIBRARIES := \
               libcutils
LOCAL_MODULE_TAGS := eng
LOCAL_MULTILIB := 64

LOCAL_MODULE:= eng_dev_test
include $(BUILD_EXECUTABLE)

