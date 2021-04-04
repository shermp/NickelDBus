#include <QHBoxLayout>
#include "NDBWidgets.h"

namespace NDB {

NDBProgressBar::NDBProgressBar(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    auto layout = new QHBoxLayout();
    prog = new QProgressBar(this);
    label = new QLabel(this);
    layout->addWidget(prog);
    layout->addWidget(label);
    prog->setStyleSheet(QString(R"(
        QProgressBar:horizontal {
            border: 1px solid gray;
            background: white;
            padding: 1px;
        }
        QProgressBar::chunk:horizontal {
            background-color: black;
            margin: 0;
        }
    )"));
    this->setLayout(layout);
    connect(prog, &QProgressBar::valueChanged, this, &NDBProgressBar::setLabel);
}

NDBProgressBar::~NDBProgressBar() {}

int NDBProgressBar::minimum() {
    return prog->minimum();
}

void NDBProgressBar::setMinimum(int min) {
    prog->setMinimum(min);
}

int NDBProgressBar::maximum() {
    return prog->maximum();
}

void NDBProgressBar::setMaximum(int max) {
    prog->setMaximum(max);
}

int NDBProgressBar::value() {
    return prog->value();
}

void NDBProgressBar::setValue(int val) {
    prog->setValue(val);
}

void NDBProgressBar::setFormat(QString const& format) {
    prog->setFormat(format);
}

QString NDBProgressBar::text() {
    return prog->text();
}

void NDBProgressBar::setLabel() {
    const QString t = prog->text();
    label->setText(t);
}

} // namespace NDB
