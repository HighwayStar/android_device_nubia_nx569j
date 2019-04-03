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

#ifndef ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H
#define ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H

#include <android/hardware/light/2.0/ILight.h>
#include <hidl/Status.h>

#include <fstream>
#include <mutex>
#include <unordered_map>

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

struct Light : public ILight {
    Light(std::pair<std::ofstream, uint32_t>&& lcd_backlight,
          std::ofstream&& nubia_blink, std::ofstream&& nubia_brightness,
          std::ofstream&& nubia_fadeParam, std::ofstream&& nubia_gradeParam,
          std::ofstream&& nubia_outn);

    // Methods from ::android::hardware::light::V2_0::ILight follow.
    Return<Status> setLight(Type type, const LightState& state) override;
    Return<void> getSupportedTypes(getSupportedTypes_cb _hidl_cb) override;

  private:
    void setAttentionLight(const LightState& state);
    void setBatteryLight(const LightState& state);
    void setButtonsBacklight(const LightState& state);
    void setLcdBacklight(const LightState& state);
    void setNotificationLight(const LightState& state);
    void setSpeakerBatteryLightLocked();
    void setSpeakerLightBlinkingLocked(const LightState& state);
    void setSpeakerLightConstLocked(const LightState& state);
    void setHomeBlinking(bool lit, bool blinking);
    void setButtonsblinking(bool lit, bool blinking);

    std::pair<std::ofstream, uint32_t> mLcdBacklight;

    std::ofstream mNubiaBlink;
    std::ofstream mNubiaBrightness;
    std::ofstream mNubiaFadeParam;
    std::ofstream mNubiaGradeParam;
    std::ofstream mNubiaOutn;

    LightState mAttentionState;
    LightState mBatteryState;
    LightState mButtonsState;
    LightState mNotificationState;

    std::unordered_map<Type, std::function<void(const LightState&)>> mLights;
    std::mutex mLock;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H
