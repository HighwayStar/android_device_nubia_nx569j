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

#define LOG_TAG "LightService"

#include "Light.h"

#include <android-base/logging.h>

namespace {

using android::hardware::light::V2_0::LightState;

static constexpr int DEFAULT_MAX_BRIGHTNESS = 255;

static constexpr int HOME_MASK = 16;
static constexpr int LEFT_MASK = 8;
static constexpr int RIGHT_MASK = 32;

static constexpr int AW_POWER_OFF = 0;
static constexpr int AW_CONST_ON = 1;
static constexpr int AW_FADE_AUTO = 3;

static uint32_t rgbToBrightness(const LightState& state) {
    uint32_t color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff)) + (150 * ((color >> 8) & 0xff)) +
            (29 * (color & 0xff))) >> 8;
}

static bool isLit(const LightState& state) {
    return (state.color & 0x00ffffff);
}

}  // anonymous namespace

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

Light::Light(std::pair<std::ofstream, uint32_t>&& lcd_backlight,
             std::ofstream&& nubia_blink, std::ofstream&& nubia_brightness,
             std::ofstream&& nubia_fadeParam, std::ofstream&& nubia_gradeParam,
             std::ofstream&& nubia_outn)
    : mLcdBacklight(std::move(lcd_backlight)),
      mNubiaBlink(std::move(nubia_blink)),
      mNubiaBrightness(std::move(nubia_brightness)),
      mNubiaFadeParam(std::move(nubia_fadeParam)),
      mNubiaGradeParam(std::move( nubia_gradeParam)),
      mNubiaOutn(std::move(nubia_outn)) {
    auto attnFn(std::bind(&Light::setAttentionLight, this, std::placeholders::_1));
    auto backlightFn(std::bind(&Light::setLcdBacklight, this, std::placeholders::_1));
    auto batteryFn(std::bind(&Light::setBatteryLight, this, std::placeholders::_1));
    auto buttonsFn(std::bind(&Light::setButtonsBacklight, this, std::placeholders::_1));
    auto notifFn(std::bind(&Light::setNotificationLight, this, std::placeholders::_1));
    mLights.emplace(std::make_pair(Type::ATTENTION, attnFn));
    mLights.emplace(std::make_pair(Type::BACKLIGHT, backlightFn));
    mLights.emplace(std::make_pair(Type::BATTERY, batteryFn));
    mLights.emplace(std::make_pair(Type::BUTTONS, buttonsFn));
    mLights.emplace(std::make_pair(Type::NOTIFICATIONS, notifFn));
}

// Methods from ::android::hardware::light::V2_0::ILight follow.
Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = mLights.find(type);

    if (it == mLights.end()) {
         LOG(ERROR) << "LIGHT_NOT_SUPPORTED";
        return Status::LIGHT_NOT_SUPPORTED;
    }

    it->second(state);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : mLights) {
        types.push_back(light.first);
    }

    _hidl_cb(types);

    return Void();
}

void Light::setAttentionLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    LOG(ERROR) << "setAttentionLight";
    mAttentionState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setLcdBacklight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);

    uint32_t brightness = rgbToBrightness(state);

    // If max panel brightness is not the default (255),
    // apply linear scaling across the accepted range.
    if (mLcdBacklight.second != DEFAULT_MAX_BRIGHTNESS) {
        int old_brightness = brightness;
        brightness = brightness * mLcdBacklight.second / DEFAULT_MAX_BRIGHTNESS;
        LOG(VERBOSE) << "scaling brightness " << old_brightness << " => " << brightness;
    }

    mLcdBacklight.first << brightness << std::endl;
}

void Light::setButtonsBacklight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    LOG(ERROR) << "setButtonsBacklight";
    mButtonsState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setBatteryLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    LOG(ERROR) << "setBatteryLight";
    mBatteryState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setNotificationLight(const LightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    LOG(ERROR) << "setNotificationLight";
    mNotificationState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setHomeBlinking(bool lit, bool blinking)
{
    LOG(ERROR) << "setHomeBlinking" << lit << blinking;
    if (lit){
	mNubiaOutn << HOME_MASK << std::endl;

	if (blinking) {
		mNubiaFadeParam << "3 0 4" << std::endl;
		mNubiaGradeParam << "0 255" << std::endl;
		mNubiaBlink <<  AW_FADE_AUTO << std::endl;
	} else {
		mNubiaFadeParam << "1 0 0" << std::endl;
		mNubiaGradeParam << "10 255" << std::endl;
		mNubiaBlink << AW_CONST_ON << std::endl;
	}
    } else {
	mNubiaOutn << HOME_MASK << std::endl;
        mNubiaFadeParam << "1 0 0" << std::endl;
        mNubiaGradeParam << "10 255" << std::endl;
	mNubiaBlink <<  AW_POWER_OFF << std::endl;
    }

}
void Light::setButtonsblinking(bool lit, bool blinking)
{
    LOG(ERROR) << "setButtonsBlinking" << lit << blinking;
    if (lit){
	mNubiaOutn << LEFT_MASK << std::endl;
	if (blinking) {
		mNubiaFadeParam << "3 0 4" << std::endl;
		mNubiaGradeParam << "0 255" << std::endl;
		mNubiaBlink <<  AW_FADE_AUTO << std::endl;
	} else {
		mNubiaFadeParam << "1 0 0" << std::endl;
		mNubiaGradeParam << "10 255" << std::endl;
		mNubiaBlink << AW_CONST_ON << std::endl;
	}
	mNubiaOutn << RIGHT_MASK << std::endl;

	if (blinking) {
		mNubiaFadeParam << "3 0 4" << std::endl;
		mNubiaGradeParam << "0 255" << std::endl;
		mNubiaBlink <<  AW_FADE_AUTO << std::endl;
	} else {
		mNubiaFadeParam << "1 0 0" << std::endl;
		mNubiaGradeParam << "10 255" << std::endl;
		mNubiaBlink << AW_CONST_ON << std::endl;
	}
    } else {
	mNubiaOutn <<  0 << std::endl;
        mNubiaFadeParam << "1 0 0" << std::endl;
        mNubiaGradeParam << "10 255" << std::endl;
	mNubiaBlink <<  AW_POWER_OFF << std::endl;
    }


}

void Light::setSpeakerBatteryLightLocked() {
    if (isLit(mNotificationState)) {
	LOG(ERROR) << "notification";
        setSpeakerLightBlinkingLocked(mNotificationState);
    } else if (isLit(mAttentionState)) {
	LOG(ERROR) << "attention";
        setSpeakerLightBlinkingLocked(mAttentionState);
    } else if (isLit(mBatteryState)) {
	LOG(ERROR) << "battery";
        setSpeakerLightConstLocked(mBatteryState);
    } else if (isLit(mButtonsState)) {
	LOG(ERROR) << "buttons";
        setSpeakerLightConstLocked(mButtonsState);
    } else {
	LOG(ERROR) << "off";
	setButtonsblinking(false, false);
	setHomeBlinking(false, false);
    }
}

void Light::setSpeakerLightBlinkingLocked(const LightState&) {

    // Disable all blinking to start
#ifdef NO_HOME_LED
    setButtonsblinking(false, false);
#else
    setHomeBlinking(false, false);
#endif

//if no home
#ifdef NO_HOME_LED
    setButtonsblinking(true, true);
#else
    setHomeBlinking(true, true);
#endif

}

void Light::setSpeakerLightConstLocked(const LightState&) {

    // Disable all blinking to start
    setButtonsblinking(false, false);
    setHomeBlinking(false, false);

    setButtonsblinking(true, false);
    setHomeBlinking(true, false);
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
