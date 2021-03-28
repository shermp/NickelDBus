#include <NickelHook.h>
#include "util.h"
#include "NDBTouchWidgets.h"

#define NDB_TW_ASSERT(obj) if (!(obj)) { return nullptr; }
#define NDB_TW_INIT_SYM(name, symbol) if(!(symbol) )

namespace NDBTouchWidgets {

    namespace NDBTouchLabel {
        TouchLabel *(*TouchLabel__TouchLabel_text)(TouchLabel *_this, QString const& text, QWidget* parent, QFlags<Qt::WindowType> flags);
        TouchLabel *(*TouchLabel__TouchLabel)(TouchLabel *_this, QWidget* parent, QFlags<Qt::WindowType> flags);

        bool initSymbols() {
            if (!TouchLabel__TouchLabel_text)
                ndbResolveSymbolRTLD("_ZN10TouchLabelC1ERK7QStringP7QWidget6QFlagsIN2Qt10WindowTypeEE", 
                                    nh_symoutptr(TouchLabel__TouchLabel_text));

            if (!TouchLabel__TouchLabel)
                ndbResolveSymbolRTLD("_ZN10TouchLabelC1EP7QWidget6QFlagsIN2Qt10WindowTypeEE", 
                                    nh_symoutptr(TouchLabel__TouchLabel));
            
            return (TouchLabel__TouchLabel_text && TouchLabel__TouchLabel);
        }

        TouchLabel* create(QString const& text, QWidget* parent, QFlags<Qt::WindowType> flags) {
            NDB_TW_ASSERT(initSymbols());
            auto tl = reinterpret_cast<TouchLabel*>(calloc(1, 128));
            NDB_TW_ASSERT(tl);
            return TouchLabel__TouchLabel_text(tl, text, parent, flags);
        }

        TouchLabel* create(QWidget* parent, QFlags<Qt::WindowType> flags) {
            NDB_TW_ASSERT(initSymbols());
            auto tl = reinterpret_cast<TouchLabel*>(calloc(1, 128));
            NDB_TW_ASSERT(tl);
            return TouchLabel__TouchLabel(tl, parent, flags);
        }
    }

    namespace NDBTouchCheckBox {
        TouchCheckBox *(*TouchCheckBox__TouchCheckBox)(TouchCheckBox* _this, QWidget* parent);

        bool initSymbols() {
            if (!TouchCheckBox__TouchCheckBox)
                ndbResolveSymbolRTLD("_ZN13TouchCheckBoxC1EP7QWidget", nh_symoutptr(TouchCheckBox__TouchCheckBox));
            
            return (TouchCheckBox__TouchCheckBox != nullptr);
        }

        TouchCheckBox* create(QWidget* parent) {
            NDB_TW_ASSERT(initSymbols());
            auto tcb = reinterpret_cast<TouchCheckBox*>(calloc(1,128));
            NDB_TW_ASSERT(tcb);
            return TouchCheckBox__TouchCheckBox(tcb, parent);
        }
    }

    namespace NDBTouchRadioButton {
        TouchRadioButton *(*TouchRadioButton__TouchRadioButton)(TouchRadioButton* _this, QWidget* parent);

        bool initSymbols() {
            if (!TouchRadioButton__TouchRadioButton)
                ndbResolveSymbolRTLD("_ZN16TouchRadioButtonC1EP7QWidget", nh_symoutptr(TouchRadioButton__TouchRadioButton));
            
            return (TouchRadioButton__TouchRadioButton != nullptr);
        }

        TouchRadioButton* create(QWidget* parent) {
            NDB_TW_ASSERT(initSymbols());
            auto trb = reinterpret_cast<TouchRadioButton*>(calloc(1,128));
            NDB_TW_ASSERT(trb);
            return TouchRadioButton__TouchRadioButton(trb, parent);
        }
    }

    namespace NDBTouchSlider {
        TouchSlider *(*TouchSlider__TouchSlider)(TouchSlider* _this, QWidget* parent);
        
        bool initSymbols() {
            if (!TouchSlider__TouchSlider)
                ndbResolveSymbolRTLD("_ZN11TouchSliderC1EP7QWidget", nh_symoutptr(TouchSlider__TouchSlider));
            
            return (TouchSlider__TouchSlider != nullptr);
        }

        TouchSlider* create(QWidget* parent) {
            NDB_TW_ASSERT(initSymbols());
            auto ts = reinterpret_cast<TouchSlider*>(calloc(1,128));
            NDB_TW_ASSERT(ts);
            return TouchSlider__TouchSlider(ts, parent);
        }
    }


    namespace NDBTouchDropDown {
        TouchDropDown *(*TouchDropDown__TouchDropDown)(TouchDropDown* _this, QWidget* parent);
        BlockTouchDropDown *(*BlockTouchDropDown__BlockTouchDropDown)(BlockTouchDropDown* _this, QWidget* parent);
        void (*TouchDropDown__addItem_loc)(TouchDropDown* _this, QString const& name, QVariant const& value, QLocale const& loc, bool prepend);
        void (*TouchDropDown__addItem)(TouchDropDown* _this, QString const& name, QVariant const& value, bool prepend);
        void (*TouchDropDown__clear)(TouchDropDown* _this);
        void (*TouchDropDown__setCurrentIndex)(TouchDropDown* _this, int index);
        QVariant (*TouchDropDown__currentData)(TouchDropDown* _this);

        bool initSymbols() {
            if (!BlockTouchDropDown__BlockTouchDropDown)
                ndbResolveSymbolRTLD("_ZN18BlockTouchDropDownC1EP7QWidget", nh_symoutptr(BlockTouchDropDown__BlockTouchDropDown));
        
            if (!TouchDropDown__TouchDropDown) 
                ndbResolveSymbolRTLD("_ZN13TouchDropDownC1EP7QWidget", nh_symoutptr(TouchDropDown__TouchDropDown));
            
            if (!TouchDropDown__addItem_loc || !TouchDropDown__addItem) {
                ndbResolveSymbolRTLD("_ZN13TouchDropDown7addItemERK7QStringRK8QVariantRK7QLocaleb", nh_symoutptr(TouchDropDown__addItem_loc));
                if (!TouchDropDown__addItem_loc)
                    ndbResolveSymbolRTLD("_ZN13TouchDropDown7addItemERK7QStringRK8QVariantb", nh_symoutptr(TouchDropDown__addItem));
            }

            if (!TouchDropDown__clear) 
                ndbResolveSymbolRTLD("_ZN13TouchDropDown5clearEv", nh_symoutptr(TouchDropDown__clear));
            
            if (!TouchDropDown__currentData)
                ndbResolveSymbolRTLD("_ZNK13TouchDropDown11currentDataEv", nh_symoutptr(TouchDropDown__currentData));
            
            if (!TouchDropDown__setCurrentIndex) {
                ndbResolveSymbolRTLD("_ZN13TouchDropDown15setCurrentIndexEi", nh_symoutptr(TouchDropDown__setCurrentIndex));
            }

            return (BlockTouchDropDown__BlockTouchDropDown && 
                    TouchDropDown__TouchDropDown && 
                    (TouchDropDown__addItem_loc || TouchDropDown__addItem) &&
                    TouchDropDown__clear &&
                    TouchDropDown__currentData &&
                    TouchDropDown__setCurrentIndex);
        }

        TouchDropDown* create(QWidget* parent, bool createBlock) {
            NDB_TW_ASSERT(initSymbols());
            void* td = calloc(1,128);
            NDB_TW_ASSERT(td);
            if (createBlock) {
                // A BlockTouchDown essentially appears to be a standard TouchDropDown with a border around it.
                // Don't know why Kobo decided it needed to be it's own class.
                return BlockTouchDropDown__BlockTouchDropDown(reinterpret_cast<BlockTouchDropDown*>(td), parent);
            } else {
                return TouchDropDown__TouchDropDown(reinterpret_cast<TouchDropDown*>(td), parent);
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
}
