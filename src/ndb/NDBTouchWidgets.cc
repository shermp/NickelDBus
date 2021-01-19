#include <NickelHook.h>
#include "util.h"
#include "NDBTouchWidgets.h"

template<typename T, typename... A>
T* createTouchWidget(const char* name, size_t sz, A...args) {
    T *(*func)(T* _this, A...);
    ndbResolveSymbolRTLD(name, nh_symoutptr(func));
    if (func) {
        T *tw = reinterpret_cast<T*>(calloc(1,sz));
        if (tw) {
            return func(tw, args...);
        }
    }
    return nullptr;
}

// template<typename T, typename... A>
// T callTouchMethod(const char* name, A...args) {
//     T (*method)(A...);
//     ndbResolveSymbolRTLD(name, nh_symoutptr(method));
//     return method(args...);
// }

TouchLabel* NDBTouchLabel::create(QString const& text, QWidget* parent, QFlags<Qt::WindowType> flags) {
    return createTouchWidget<TouchLabel>("_ZN10TouchLabelC1ERK7QStringP7QWidget6QFlagsIN2Qt10WindowTypeEE", 128, text, parent, flags);
}

TouchLabel* NDBTouchLabel::create(QWidget* parent, QFlags<Qt::WindowType> flags) {
    return createTouchWidget<TouchLabel>("_ZN10TouchLabelC2EP7QWidget6QFlagsIN2Qt10WindowTypeEE", 128, parent, flags);
}

TouchCheckBox* NDBTouchCheckBox::create(QWidget* parent) {
    return createTouchWidget<TouchCheckBox>("_ZN13TouchCheckBoxC1EP7QWidget", 128, parent);
}

TouchRadioButton* NDBTouchRadioButton::create(QWidget* parent) {
    return createTouchWidget<TouchRadioButton>("_ZN16TouchRadioButtonC1EP7QWidget", 128, parent);
}

TouchSlider* NDBTouchSlider::create(QWidget* parent) {
    return createTouchWidget<TouchSlider>("_ZN11TouchSliderC1EP7QWidget", 128, parent);
}

namespace NDBTouchDropDown {
    void (*TouchDropDown__addItem_loc)(TouchDropDown* _this, QString const& name, QVariant const& value, QLocale const& loc, bool prepend);
    void (*TouchDropDown__addItem)(TouchDropDown* _this, QString const& name, QVariant const& value, bool prepend);
    void (*TouchDropDown__clear)(TouchDropDown* _this);
    void (*TouchDropDown__setCurrentIndex)(TouchDropDown* _this, int index);
    QVariant (*TouchDropDown__currentData)(TouchDropDown* _this);

    TouchDropDown* create(QWidget* parent, bool createBlock) {
        ndbResolveSymbolRTLD("_ZN13TouchDropDown7addItemERK7QStringRK8QVariantRK7QLocaleb", nh_symoutptr(TouchDropDown__addItem_loc));
        if (!TouchDropDown__addItem_loc) {
            ndbResolveSymbolRTLD("_ZN13TouchDropDown7addItemERK7QStringRK8QVariantb", nh_symoutptr(TouchDropDown__addItem));
            if (!TouchDropDown__addItem) {
                return nullptr;
            }
        }
        ndbResolveSymbolRTLD("_ZN13TouchDropDown5clearEv", nh_symoutptr(TouchDropDown__clear));
        ndbResolveSymbolRTLD("_ZN13TouchDropDown15setCurrentIndexEi", nh_symoutptr(TouchDropDown__setCurrentIndex));
        ndbResolveSymbolRTLD("_ZNK13TouchDropDown11currentDataEv", nh_symoutptr(TouchDropDown__currentData));
        if (!TouchDropDown__clear || !TouchDropDown__setCurrentIndex || !TouchDropDown__currentData) {
            return nullptr;
        }
        if (createBlock) {
            // A BlockTouchDown essentially appears to be a standard TouchDropDown with a border around it.
            // Don't know why Kobo decided it needed to be it's own class.
            return createTouchWidget<TouchDropDown>("_ZN18BlockTouchDropDownC1EP7QWidget", 256, parent);
        } else {
            return createTouchWidget<TouchDropDown>("_ZN13TouchDropDownC1EP7QWidget", 256, parent);
        }
}

    void addItem(TouchDropDown* _this, QString const& name, QVariant const& value, bool prepend) {
        if (TouchDropDown__addItem_loc) {
            QLocale loc;
            return TouchDropDown__addItem_loc(_this, name, value, loc, prepend);
        } else {
            return TouchDropDown__addItem(_this, name, value, prepend);
        }
    }

    void clear(TouchDropDown* _this) {
        return TouchDropDown__clear(_this);
    }

    void setCurrentIndex(TouchDropDown* _this, int index) {
        return TouchDropDown__setCurrentIndex(_this, index);
    }

    QVariant currentData(TouchDropDown* _this) {
        return TouchDropDown__currentData(_this);
    }
}
