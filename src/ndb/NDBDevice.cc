#include <NickelHook.h>
#include "NDBDevice.h"
#include "util.h"

NDBDevice::NDBDevice() {
    initResult = Ok;
    Device *(*Device__getCurrentDevice)();
    ndbResolveSymbolRTLD("_ZN6Device16getCurrentDeviceEv", nh_symoutptr(Device__getCurrentDevice));
    if (!Device__getCurrentDevice) {
        initResult = SymbolError;
        return;
    }
    if (!device) {
        initResult = NullError;
        return;
    }
}

template<typename T>
T NDBDevice::callDeviceMethod(const char* symbol) {
    T t = T();
    T (*method)(Device*);
    ndbResolveSymbolRTLD(symbol, nh_symoutptr(method));
    if (method) {
        return method(device);
    } else {
        return t;
    }
}

template <typename T, typename... A>
T NDBDevice::callDeviceMethodArgs(const char* symbol, A... args) {
    T t = T();
    T (*method)(Device*, A...);
    ndbResolveSymbolRTLD(name, nh_symoutptr(method));
    if (method) {
        return method(device, args...);
    } else {
        return t;
    }
}

bool NDBDevice::isAlyssum() {
    return callDeviceMethod<bool>("_ZNK6Device9isAlyssumEv");
}

bool NDBDevice::isDahlia() {
    return callDeviceMethod<bool>("_ZNK6Device8isDahliaEv");
}

bool NDBDevice::isDaylight(bool p1) {
    return callDeviceMethodArgs<bool>("_ZNK6Device10isDaylightEb", p1);
}

bool NDBDevice::isDesktop() {
    return callDeviceMethod<bool>("_ZNK6Device9isDesktopEv");
}

bool NDBDevice::isDragon(bool p1) {
    return callDeviceMethodArgs<bool>("_ZNK6Device8isDragonEb", p1);
}

bool NDBDevice::isFrost() {
    return callDeviceMethod<bool>("_ZNK6Device7isFrostEv");
    
}
bool NDBDevice::isKraken() {
    return callDeviceMethod<bool>("_ZNK6Device8isKrakenEv");
}

bool NDBDevice::isLuna() {
    return callDeviceMethod<bool>("_ZNK6Device6isLunaEv");
}

bool NDBDevice::isNova() {
    return callDeviceMethod<bool>("_ZNK6Device6isNovaEv");
}

bool NDBDevice::isPhoenix(bool p1) {
    return callDeviceMethodArgs<bool>("_ZNK6Device9isPhoenixEb", p1);
}

bool NDBDevice::isPika() {
    return callDeviceMethod<bool>("_ZNK6Device6isPikaEv");
}

bool NDBDevice::isPixie() {
    return callDeviceMethod<bool>("_ZNK6Device7isPixieEv");
}

bool NDBDevice::isSnow() {
    return callDeviceMethod<bool>("_ZNK6Device6isSnowEv");
}

bool NDBDevice::isStar() {
    return callDeviceMethod<bool>("_ZNK6Device6isStarEv");
}

bool NDBDevice::isStorm() {
    return callDeviceMethod<bool>("_ZNK6Device7isStormEv");
}

bool NDBDevice::isTrilogy(bool p1) {
    return callDeviceMethodArgs<bool>("_ZNK6Device9isTrilogyEb", p1);
}
