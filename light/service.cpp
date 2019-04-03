/*
 * Copyright (C) 2019 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.light@2.0-service.nubia"

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Errors.h>

#include "Light.h"

// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

// Generated HIDL files
using android::hardware::light::V2_0::ILight;
using android::hardware::light::V2_0::implementation::Light;

const static std::string kLcdBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
const static std::string kLcdMaxBacklightPath = "/sys/class/leds/lcd-backlight/max_brightness";
const static std::string kNubiaBlinkPath = "/sys/class/leds/nubia_led/blink_mode";
const static std::string kNubiaBrightnessPath = "/sys/class/leds/nubia_led/brightness";
const static std::string kNubiaFadeParamPath = "/sys/class/leds/nubia_led/fade_parameter";
const static std::string kNubiaGradeParamPath = "/sys/class/leds/nubia_led/grade_parameter";
const static std::string kNubiaOutnPath = "/sys/class/leds/nubia_led/outn";

int main() {
    uint32_t lcdMaxBrightness = 255;

    std::ofstream lcdBacklight(kLcdBacklightPath);
    if (!lcdBacklight) {
        LOG(ERROR) << "Failed to open " << kLcdBacklightPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ifstream lcdMaxBacklight(kLcdMaxBacklightPath);
    if (!lcdMaxBacklight) {
        LOG(ERROR) << "Failed to open " << kLcdMaxBacklightPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    } else {
        lcdMaxBacklight >> lcdMaxBrightness;
    }

    std::ofstream nubiaBlink(kNubiaBlinkPath);
    if (!nubiaBlink) {
        LOG(ERROR) << "Failed to open " << kNubiaBlinkPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream nubiaBrightness(kNubiaBrightnessPath);
    if (!nubiaBrightness) {
        LOG(ERROR) << "Failed to open " << kNubiaBrightnessPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream nubiaFadeParam(kNubiaFadeParamPath);
    if (!nubiaFadeParam) {
        LOG(ERROR) << "Failed to open " << kNubiaFadeParamPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }
    std::ofstream nubiaGradeParam(kNubiaGradeParamPath);
    if (!nubiaGradeParam) {
        LOG(ERROR) << "Failed to open " << kNubiaGradeParamPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }
    std::ofstream nubiaOutn(kNubiaOutnPath);
    if (!nubiaOutn) {
        LOG(ERROR) << "Failed to open " << kNubiaOutnPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    android::sp<ILight> service = new Light(
            {std::move(lcdBacklight), lcdMaxBrightness},
            std::move(nubiaBlink), std::move(nubiaBrightness), 
            std::move(nubiaFadeParam), std::move(nubiaGradeParam),
            std::move(nubiaOutn));

    configureRpcThreadpool(1, true);

    android::status_t status = service->registerAsService();

    if (status != android::OK) {
        LOG(ERROR) << "Cannot register Light HAL service";
        return 1;
    }

    LOG(INFO) << "Light HAL Ready.";
    joinRpcThreadpool();
    // Under normal cases, execution will not reach this line.
    LOG(ERROR) << "Light HAL failed to join thread pool.";
    return 1;
}
