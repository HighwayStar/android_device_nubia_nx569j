#
# Copyright (C) 2016 The CyanogenMod Project
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
#

# Overlays
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# call the proprietary setup
$(call inherit-product-if-exists, vendor/nubia/nx569j/nx569j-vendor.mk)

PRODUCT_COPY_FILES +=  \
    $(LOCAL_PATH)/audio/mixer_paths_mtp.xml:system/etc/mixer_paths_mtp.xml \
    $(LOCAL_PATH)/audio/audio_platform_info_extcodec.xml:system/etc/audio_platform_info_extcodec.xml \
    $(LOCAL_PATH)/audio/audio_platform_info.xml:system/etc/audio_platform_info.xml

# FM
PRODUCT_PACKAGES += \
    FMRadio \
    libfmjni

# Ramdisk nubia specific
PRODUCT_PACKAGES += \
    init.nb.device.rc

# Inherit from msm8956-common
$(call inherit-product, device/nubia/msm8976-common/msm8976.mk)
