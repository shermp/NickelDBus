#ifndef NDB_CONFIRM_DLG_H
#define NDB_CONFIRM_DLG_H
#include <QObject>
#include <QDialog>
#include <QFrame>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>
#include <QPointer>
#include <QCheckBox>
#include <QLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include "NDBTouchWidgets.h"

typedef QDialog ConfirmationDialog;
typedef int KeyboardScript;
typedef QLineEdit TouchLineEdit;
typedef QWidget N3ConfirmationTextEditField;
typedef QCheckBox TouchCheckBox;

namespace NDB {

class NDBCfmDlg : public QObject {
    Q_OBJECT
    public:
        enum result {Ok, NotImplemented, InitError, SymbolError, NullError, ForbiddenError, ParamError, ConnError};
        enum dialogType {TypeStd, TypeLineEdit, TypeAdvanced};
        enum layoutType {VertLayout, HorLayout, FormLayout};
        enum result initResult;
        NDBCfmDlg(QObject* parent);
        ~NDBCfmDlg();
        QString errString;
        QPointer<ConfirmationDialog> dlg;
        enum result createDialog(
            enum dialogType dlgType,
            QString const& title, 
            QString const& body, 
            QString const& acceptText, 
            QString const& rejectText, 
            bool tapOutsideClose
        );
        enum result showDialog();
        enum result closeDialog();
        enum result updateBody(QString const& body);
        void setPassword(bool isPassword);
        QString getText();
        void setText(QString const& text);
        enum result advAddLayout(enum layoutType lt);
        enum result advAddCheckbox(QString const& name, QString const& label, bool checked);
        //enum result advUpdateCheckbox(QString const& name, bool checked);
        enum result advAddSlider(QString const& name, QString const& label, int min, int max, int val);
        //enum result advUpdateSlider(QString const& name, int val);
        enum result advAddDropDown(QString const& name, QString const& label, QStringList items, bool allowAdditionAndRemoval);
        //enum result advAddLineEdit(QString const& name, QString const& label, QString const& placeholder);
        enum result advAddLineEdit(QString const& name, QString const& label, bool autoFormatCaps);
        enum result advAddTextEdit(QString const& name, QString const& label, bool autoFormatCaps);
        enum result advAddDatePicker(QString const& name, QString const& label, QDate init);
        enum result advAddTimePicker(QString const& name, QString const& label, QTime init);
        enum result advGetJSON(QString& json);
    protected Q_SLOTS:
        void onLineTextEditTapped();
        void onCommitRequested();
    private:
        QString styleSheet;
        struct {
            ConfirmationDialog *(*ConfirmationDialogFactory_getConfirmationDialog)(QWidget*);
            ConfirmationDialog *(*ConfirmationDialogFactory_showTextEditDialog)(QString const& title);
            void (*ConfirmationDialog__setTitle)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setAcceptButtonText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setRejectButtonText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__showCloseButton)(ConfirmationDialog* _this, bool show);
            void (*ConfirmationDialog__setRejectOnOutsideTap)(ConfirmationDialog* _this, bool setReject);
            void (*ConfirmationDialog__addWidget)(ConfirmationDialog* _this, QWidget* w);
            void (*ConfirmationDialog__setContent)(ConfirmationDialog* _this, QWidget* content);
            KeyboardFrame *(*ConfirmationDialog__keyboardFrame)(ConfirmationDialog* _this);
            N3ConfirmationTextEditField *(*N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS)(
                N3ConfirmationTextEditField* _this, 
                ConfirmationDialog* dlg, 
                KeyboardScript ks);
            N3ConfirmationTextEditField *(*N3ConfirmationTextEditField__N3ConfirmationTextEditField)(
                N3ConfirmationTextEditField* _this, 
                ConfirmationDialog* dlg
            );
            TouchLineEdit *(*N3ConfirmationTextEditField__textEdit)(N3ConfirmationTextEditField* _this);
        } symbols;
        enum dialogType currActiveType;
        QPointer<TouchLineEdit> tle;
        QPointer<N3ConfirmationTextEditField> tef;
        N3ConfirmationTextEditField* createTextEditField();
        QPointer<QFrame> advContent;
        QPointer<QVBoxLayout> advMainLayout;
        QPointer<QLayout> advActiveLayout;
        void connectStdSignals();
        void addWidgetToFrame(QString const& label, QWidget* widget);
};

} // namespace NDB

#endif // NDB_CONFIRM_DLG_H
