#include <QHBoxLayout>
#include "NDBWidgets.h"

namespace NDB {

/*!
 * \brief Create a progress bar
 */
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

/*!
 * \brief Destroy a progress bar
 */
NDBProgressBar::~NDBProgressBar() {}

/*!
 * \brief Get the minimum value of a progress bar
 */
int NDBProgressBar::minimum() {
    return prog->minimum();
}

/*!
 * \brief Set the minimum value of a progress bar
 *
 * Sets the minimum value of a progress bar to \a min
 */
void NDBProgressBar::setMinimum(int min) {
    prog->setMinimum(min);
}

/*!
 * \brief Get the maximum value of a progress bar
 */
int NDBProgressBar::maximum() {
    return prog->maximum();
}

/*!
 * \brief Set the maximum value of a progress bar
 *
 * Sets the maximum value of a progress bar to \a max
 */
void NDBProgressBar::setMaximum(int max) {
    prog->setMaximum(max);
}

/*!
 * \brief Get the current value of a progress bar
 */
int NDBProgressBar::value() {
    return prog->value();
}

/*!
 * \brief Set the current value of a progress bar
 *
 * Sets the current value of a progress bar to \a val
 */
void NDBProgressBar::setValue(int val) {
    prog->setValue(val);
}

/*!
 * \brief Set the label format of a progress bar
 *
 * Sets the format of the progress bar label to 
 * \a format, which should use the same format
 * specification as a \l QProgressBar
 */
void NDBProgressBar::setFormat(QString const& format) {
    prog->setFormat(format);
}

/*!
 * \brief Get the text of a progress bar
 */
QString NDBProgressBar::text() {
    return prog->text();
}

/*!
 * \brief Set the label of a progress bar
 *
 * Sets the label of a progress bar to the text value
 * of the underlying \l QProgressBar
 */
void NDBProgressBar::setLabel() {
    const QString t = prog->text();
    label->setText(t);
}

} // namespace NDB
