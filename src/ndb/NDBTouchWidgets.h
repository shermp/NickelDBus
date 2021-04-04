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
#include <QDate>
#include <QTime>

typedef QLabel TouchLabel;
typedef QWidget TouchDropDown;
typedef TouchDropDown BlockTouchDropDown;
typedef QCheckBox TouchCheckBox;
typedef QRadioButton TouchRadioButton;
typedef QSlider TouchSlider;

typedef QWidget N3DatePicker;
typedef QWidget N3TimePicker;

// Keyboard stuff
typedef QObject SearchKeyboardController;
typedef int KeyboardScript;
typedef QFrame KeyboardFrame;
typedef QObject KeyboardReceiver;
typedef QLineEdit TouchLineEdit;
typedef QFrame TouchTextEdit;

namespace NDB {

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

    namespace NDBDateTime {
        bool initSymbols();

        N3DatePicker* create(QWidget* parent, QDate init);
        N3TimePicker* create(QWidget* parent, QTime init);

        QDate getDate(N3DatePicker* _this);
        QTime getTime(N3TimePicker* _this);
    }

    namespace NDBKeyboard {
        bool initSymbols();

        TouchLineEdit* createLineEdit(QWidget* parent, bool autoFormatCaps);
        TouchTextEdit* createTextEdit(QWidget* parent, bool autoFormatCaps);
        QTextEdit* textEdit(TouchTextEdit* tte);
        KeyboardReceiver* getKeyboardReciever(QWidget* lte);

        SearchKeyboardController* createKeyboard(KeyboardFrame* _this, KeyboardScript script, QLocale const& loc);
        void setReceiver(SearchKeyboardController* _this, KeyboardReceiver* receiver);
        void setMultilineEntry(SearchKeyboardController* _this, bool enabled);
        void setText(TouchLineEdit* tle, QString const& text);
        void setText(TouchTextEdit* tte, QString const& text);
    }
}

} // namespace NDB
#endif // NDB_TOUCH_WIDGETS_H
