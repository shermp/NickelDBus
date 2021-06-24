#ifndef NDB_WIDGETS_H
#define NDB_WIDGETS_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>

namespace NDB {

class NDBProgressBar : public QWidget 
{
    Q_OBJECT
    public:
        NDBProgressBar(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
        ~NDBProgressBar();
        int minimum();
        void setMinimum(int min);
        int maximum();
        void setMaximum(int max);
        int value();
        void setValue(int val);
        void setFormat(QString const& format);
        QString text();
    public Q_SLOTS:
        void setLabel();
    private:
        QProgressBar* prog;
        QLabel* label;
};

} // namespace NDB

#endif // NDB_WIDGETS_H
