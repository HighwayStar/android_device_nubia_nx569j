#
# Copyright (C) 2018 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# inherit from common msm8976-common
-include device/nubia/msm8976-common/BoardConfigCommon.mk

DEVICE_PATH := device/nubia/nx569j

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(DEVICE_PATH)/bluetooth

# CMHW
BOARD_HARDWARE_CLASS += \
    $(DEVICE_PATH)/cmhw

# FM
BOARD_HAVE_QCOM_FM := true
TARGET_QCOM_NO_FM_FIRMWARE := true

# Kernel
TARGET_KERNEL_CONFIG := lineageos_nx569j_defconfig

# Partitions
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 4294967296
BOARD_USERDATAIMAGE_PARTITION_SIZE := 57033595904

# Power
TARGET_TAP_TO_WAKE_NODE := "/sys/class/input/input1/wake_gesture"

# Sepolicy
BOARD_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy

# inherit from the proprietary version
-include vendor/nubia/nx569j/BoardConfigVendor.mk
