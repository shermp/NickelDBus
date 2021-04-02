#include <NickelHook.h>
#include <QTextEdit>
#include <QLineEdit>
#include "util.h"
#include "NDBTouchWidgets.h"

#define NDB_TW_ASSERT(obj) if (!(obj)) { return nullptr; }
#define NDB_TW_INIT_SYM(name, symbol) if(!(symbol) )

#define NDB_TW_ASSERT_SYM(name, symbol) if (!(symbol)) { \
    resolveSymbolRTLD((name), nh_symoutptr((symbol))); \
    if (!(symbol)) { return false; } \
}

#define NDB_TW_ASSERT_MULTI_SYM(name, symbol, name2, symbol2) if (!(symbol) || !(symbol2)) { \
    resolveSymbolRTLD((name), nh_symoutptr((symbol))); \
    if (!(symbol)) { \
        resolveSymbolRTLD((name2), nh_symoutptr((symbol2))); \
            if (!(symbol2)) { return false; } \
    } \
}

namespace NDB {

namespace NDBTouchWidgets {

    namespace NDBTouchLabel {
        TouchLabel *(*TouchLabel__TouchLabel_text)(TouchLabel *_this, QString const& text, QWidget* parent, QFlags<Qt::WindowType> flags);
        TouchLabel *(*TouchLabel__TouchLabel)(TouchLabel *_this, QWidget* parent, QFlags<Qt::WindowType> flags);

        bool initSymbols() {
            NDB_TW_ASSERT_SYM("_ZN10TouchLabelC1ERK7QStringP7QWidget6QFlagsIN2Qt10WindowTypeEE", TouchLabel__TouchLabel_text);
            NDB_TW_ASSERT_SYM("_ZN10TouchLabelC1EP7QWidget6QFlagsIN2Qt10WindowTypeEE", TouchLabel__TouchLabel);
            return true;
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
            NDB_TW_ASSERT_SYM("_ZN13TouchCheckBoxC1EP7QWidget", TouchCheckBox__TouchCheckBox);
            return true;
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
            NDB_TW_ASSERT_SYM("_ZN16TouchRadioButtonC1EP7QWidget", TouchRadioButton__TouchRadioButton);
            return true;
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
            NDB_TW_ASSERT_SYM("_ZN11TouchSliderC1EP7QWidget", TouchSlider__TouchSlider);
            return true;
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
            NDB_TW_ASSERT_SYM("_ZN18BlockTouchDropDownC1EP7QWidget", BlockTouchDropDown__BlockTouchDropDown);
            NDB_TW_ASSERT_SYM("_ZN13TouchDropDownC1EP7QWidget", TouchDropDown__TouchDropDown);
            NDB_TW_ASSERT_MULTI_SYM("_ZN13TouchDropDown7addItemERK7QStringRK8QVariantRK7QLocaleb", 
                TouchDropDown__addItem_loc,
                "_ZN13TouchDropDown7addItemERK7QStringRK8QVariantb",
                TouchDropDown__addItem
            );
            NDB_TW_ASSERT_SYM("_ZN13TouchDropDown5clearEv", TouchDropDown__clear);
            NDB_TW_ASSERT_SYM("_ZNK13TouchDropDown11currentDataEv", TouchDropDown__currentData);
            NDB_TW_ASSERT_SYM("_ZN13TouchDropDown15setCurrentIndexEi", TouchDropDown__setCurrentIndex);
            return true;
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

    namespace NDBDateTime {
        N3DatePicker *(*N3DatePicker__N3DatePicker)(N3DatePicker* _this, QWidget* parent, QDate init);
        N3TimePicker *(*N3TimePicker__N3TimePicker)(N3TimePicker* _this, QWidget* parent, QTime init);

        QDate (*N3DatePicker__getDate)(N3DatePicker* _this);
        QTime (*N3TimePicker__getTime)(N3TimePicker* _this);

        bool initSymbols() {
            NDB_TW_ASSERT_SYM("_ZN12N3DatePickerC1EP7QWidget5QDate", N3DatePicker__N3DatePicker);
            NDB_TW_ASSERT_SYM("_ZN12N3TimePickerC1EP7QWidget5QTime", N3TimePicker__N3TimePicker);
            NDB_TW_ASSERT_SYM("_ZNK12N3DatePicker7getDateEv", N3DatePicker__getDate);
            NDB_TW_ASSERT_SYM("_ZNK12N3TimePicker7getTimeEv", N3TimePicker__getTime);
            return true;
        }

        N3DatePicker* create(QWidget* parent, QDate init) {
            NDB_TW_ASSERT(initSymbols());
            auto d = reinterpret_cast<N3DatePicker*>(calloc(1,128));
            NDB_TW_ASSERT(d);
            return N3DatePicker__N3DatePicker(d, parent, init);
        }

        N3TimePicker* create(QWidget* parent, QTime init) {
            NDB_TW_ASSERT(initSymbols());
            auto t = reinterpret_cast<N3TimePicker*>(calloc(1,128));
            NDB_TW_ASSERT(t);
            return N3TimePicker__N3TimePicker(t, parent, init);
        }

        QDate getDate(N3DatePicker* _this) {
            return N3DatePicker__getDate(_this);
        }

        QTime getTime(N3TimePicker* _this) {
            return N3TimePicker__getTime(_this);
        }
    }

    namespace NDBKeyboard {
        KeyboardReceiver *(*KeyboardReceiver__KeyboardReceiver_lineEdit)(KeyboardReceiver* _this, QLineEdit* line, bool autoFormatCaps);
        KeyboardReceiver *(*KeyboardReceiver__KeyboardReceiver_textEdit)(KeyboardReceiver* _this, QTextEdit* text, bool autoFormatCaps);
        SearchKeyboardController *(*KeyboardFrame_createKeyboard)(KeyboardFrame* _this, KeyboardScript script, QLocale const& loc);
        void (*SearchKeyboardController__setReceiver)(SearchKeyboardController* _this, KeyboardReceiver* receiver, bool umm);
        void (*SearchKeyboardController__setMultiLineEntry)(SearchKeyboardController* _this, bool enabled);
        TouchLineEdit *(*TouchLineEdit__TouchLineEdit)(TouchLineEdit* _this, QWidget* parent);
        TouchTextEdit *(*TouchTextEdit__TouchTextEdit)(TouchTextEdit* _this, QWidget* parent);
        QTextEdit *(*TouchTextEdit__textEdit)(TouchTextEdit* _this);

        bool initSymbols() {
            NDB_TW_ASSERT_SYM("_ZN16KeyboardReceiverC1EP9QLineEditb", KeyboardReceiver__KeyboardReceiver_lineEdit);
            NDB_TW_ASSERT_SYM("_ZN16KeyboardReceiverC1EP9QTextEditb", KeyboardReceiver__KeyboardReceiver_textEdit);
            NDB_TW_ASSERT_SYM("_ZN13KeyboardFrame14createKeyboardE14KeyboardScriptRK7QLocale", KeyboardFrame_createKeyboard);
            NDB_TW_ASSERT_SYM("_ZN24SearchKeyboardController11setReceiverEP16KeyboardReceiverb", SearchKeyboardController__setReceiver);
            NDB_TW_ASSERT_SYM("_ZN24SearchKeyboardController17setMultiLineEntryEb", SearchKeyboardController__setMultiLineEntry);
            NDB_TW_ASSERT_SYM("_ZN13TouchLineEditC1EP7QWidget", TouchLineEdit__TouchLineEdit);
            NDB_TW_ASSERT_SYM("_ZN13TouchTextEditC1EP7QWidget", TouchTextEdit__TouchTextEdit);
            NDB_TW_ASSERT_SYM("_ZN13TouchTextEdit8textEditEv", TouchTextEdit__textEdit);
            return true;
        }

        TouchLineEdit* createLineEdit(QWidget* parent, bool autoFormatCaps) {
            NDB_TW_ASSERT(initSymbols());
            auto tle = reinterpret_cast<TouchLineEdit*>(calloc(1,sizeof(QLineEdit) * 4));
            NDB_TW_ASSERT(tle);
            TouchLineEdit__TouchLineEdit(tle, parent);
            auto tlr = reinterpret_cast<KeyboardReceiver*>(calloc(1,sizeof(QFrame) * 4));
            NDB_TW_ASSERT(tlr);
            NDB_TW_ASSERT(KeyboardReceiver__KeyboardReceiver_lineEdit(tlr, tle, autoFormatCaps));
            return tle;
        }

        TouchTextEdit* createTextEdit(QWidget* parent, bool autoFormatCaps) {
            NDB_TW_ASSERT(initSymbols());
            auto tte = reinterpret_cast<TouchTextEdit*>(calloc(1,sizeof(QTextEdit) * 4));
            NDB_TW_ASSERT(tte);
            TouchTextEdit__TouchTextEdit(tte, parent);
            textEdit(tte)->clear();
            auto ttr = reinterpret_cast<KeyboardReceiver*>(calloc(1,sizeof(QFrame) * 4));
            NDB_TW_ASSERT(ttr);
            NDB_TW_ASSERT(KeyboardReceiver__KeyboardReceiver_textEdit(ttr, textEdit(tte), autoFormatCaps));
            return tte;
        }

        QTextEdit* textEdit(TouchTextEdit* tte) {
            NDB_TW_ASSERT(initSymbols());
            return TouchTextEdit__textEdit(tte);
        }

        /* 
         * This is a lot of work just to get a pointer! I currently can't think
         * of a better way to do it without keeping track of the KeyboardReceiver 
         * separately, which I would rather not do.
         */
        KeyboardReceiver* getKeyboardReciever(QWidget* lte) {
            NDB_DEBUG("getting receiver");
            auto tle = qobject_cast<TouchLineEdit*>(lte);
            auto tte = qobject_cast<TouchTextEdit*>(lte);
            QObjectList c;
            if (tte) {
                QTextEdit *qte = TouchTextEdit__textEdit(tte);
                c = qte->children();
            } else if (tle) {
                c = tle->children();
            } else {
                nh_log("getKeyboardReciever: parameter is not TouchLineEdit or TouchTextEdit");
                return nullptr;
            }
            NDB_DEBUG("there are %d children", c.size());
            for (int i = 0; i < c.size(); ++i) {
                QObject *o = c.at(i);
                NDB_DEBUG("className at %d is '%s'", i, o->metaObject()->className());
                if (o->metaObject()->className() == QString("KeyboardReceiver")) {
                    NDB_DEBUG("KeyboardReceiver found at position %d", i);
                    return qobject_cast<KeyboardReceiver*>(o);
                }
            }
            NDB_DEBUG("KeyboardReceiver not found");
            return nullptr;
        }

        SearchKeyboardController* createKeyboard(KeyboardFrame* _this, KeyboardScript script, QLocale const& loc) {
            return KeyboardFrame_createKeyboard(_this, script, loc);
        }

        void setReceiver(SearchKeyboardController* _this, KeyboardReceiver* receiver) {
            return SearchKeyboardController__setReceiver(_this, receiver, true);
        }

        void setMultilineEntry(SearchKeyboardController* _this, bool enabled) {
            return SearchKeyboardController__setMultiLineEntry(_this, enabled);
        }
    }
}

} // namespace NDB