#ifndef NDB_TOUCH_WIDGETS_H
#define NDB_TOUCH_WIDGETS_H

#include <Qt>
#include <QWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QFlags>
#include <QVariant>
#include <QLocale>
#include <QSet>
#include <QSlider>

typedef QWidget TouchLabel;
typedef QWidget TouchDropDown;
typedef QCheckBox TouchCheckBox;
typedef QRadioButton TouchRadioButton;
typedef QSlider TouchSlider;

namespace NDBTouchLabel {
    TouchLabel* create(QString const& text, QWidget* parent, QFlags<Qt::WindowType> flags);
    TouchLabel* create(QWidget* parent, QFlags<Qt::WindowType> flags);
}

namespace NDBTouchDropDown {
    TouchDropDown* create(QWidget* parent, bool createBlock);
    void addItem(TouchDropDown* _this, QString const& name, QVariant const& value, bool prepend);
    void clear(TouchDropDown* _this);
    void setCurrentIndex(TouchDropDown* _this, int index);
    QVariant currentData(TouchDropDown* _this);
}

namespace NDBTouchCheckBox {
    TouchCheckBox* create(QWidget* parent);
}

namespace NDBTouchRadioButton {
    TouchRadioButton* create(QWidget* parent);
}

namespace NDBTouchSlider {
    TouchSlider* create(QWidget* parent);
}
#endif // NDB_TOUCH_WIDGETS_H
