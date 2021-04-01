#ifndef NDB_DEVICE_H
#define NDB_DEVICE_H

typedef void Device;

namespace NDB {

class NDBDevice {
    public:
        enum result {Ok, NotImplemented, InitError, SymbolError, NullError, ForbiddenError, ParamError};
        enum result initResult;
        NDBDevice();
        ~NDBDevice();
        bool isAlyssum();
        bool isDahlia();
        bool isDaylight(bool p1);
        bool isDesktop();
        bool isDragon(bool p1);
        bool isFrost();
        bool isKraken();
        bool isLuna();
        bool isNova();
        bool isPhoenix(bool p1);
        bool isPika();
        bool isPixie();
        bool isSnow();
        bool isStar();
        bool isStorm();
        bool isTrilogy(bool p1);
    private:
        Device *device;
        template <typename T>
        T callDeviceMethod(const char* symbol);
        template <typename T, typename... A>
        T callDeviceMethodArgs(const char* symbol, A... args);
};

} // namespace NDB
#endif //NDB_DEVICE_H