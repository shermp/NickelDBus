#ifndef NDB_TOUCH_WIDGETS_H
#define NDB_TOUCH_WIDGETS_H

#include <Qt>
#include <QWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QFlags>
#include <QVariant>
#include <QLocale>
#include <QLabel>
#include <QSet>
#include <QSlider>

typedef QLabel TouchLabel;
typedef QWidget TouchDropDown;
typedef TouchDropDown BlockTouchDropDown;
typedef QCheckBox TouchCheckBox;
typedef QRadioButton TouchRadioButton;
typedef QSlider TouchSlider;

// Keyboard stuff
typedef QObject SearchKeyboardController;
typedef int KeyboardScript;
typedef QFrame KeyboardFrame;
typedef QObject KeyboardReceiver;
typedef QLineEdit TouchLineEdit;
typedef QFrame TouchTextEdit;

namespace NDBTouchWidgets {
    namespace NDBTouchLabel {
        bool initSymbols();

        TouchLabel* create(QString const& text, QWidget* parent, QFlags<Qt::WindowType> flags);
        TouchLabel* create(QWidget* parent, QFlags<Qt::WindowType> flags);
    }

    namespace NDBTouchDropDown {
        bool initSymbols();
        
        TouchDropDown* create(QWidget* parent, bool createBlock);
        void addItem(TouchDropDown* _this, QString const& name, QVariant const& value, bool prepend);
        void clear(TouchDropDown* _this);
        void setCurrentIndex(TouchDropDown* _this, int index);
        QVariant currentData(TouchDropDown* _this);
    }

    namespace NDBTouchCheckBox {
        bool initSymbols();

        TouchCheckBox* create(QWidget* parent);
    }

    namespace NDBTouchRadioButton {
        bool initSymbols();

        TouchRadioButton* create(QWidget* parent);
    }

    namespace NDBTouchSlider {
        bool initSymbols();

        TouchSlider* create(QWidget* parent);
    }

    namespace NDBKeyboard {
        bool initSymbols();

        TouchLineEdit* createLineEdit(QWidget* parent);
        TouchTextEdit* createTextEdit(QWidget* parent);
        QTextEdit* textEdit(TouchTextEdit* tte);
        KeyboardReceiver* getKeyboardReciever(QWidget* lte);

        SearchKeyboardController* createKeyboard(KeyboardFrame* _this, KeyboardScript script, QLocale const& loc);
        void setReceiver(SearchKeyboardController* _this, KeyboardReceiver* receiver);
        void setMultilineEntry(SearchKeyboardController* _this, bool enabled);
    }
}
#endif // NDB_TOUCH_WIDGETS_H
