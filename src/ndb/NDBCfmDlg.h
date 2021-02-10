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
#include <QVBoxLayout>

typedef QDialog ConfirmationDialog;
typedef int KeyboardScript;
typedef QLineEdit TouchLineEdit;
typedef QWidget N3ConfirmationTextEditField;
typedef QCheckBox TouchCheckBox;

class NDBCfmDlg : public QObject {
    Q_OBJECT
    public:
        enum result {Ok, NotImplemented, InitError, SymbolError, NullError, ForbiddenError, ParamError};
        enum dialogType {TypeStd, TypeLineEdit, TypeAdvanced};
        enum result initResult;
        NDBCfmDlg(QObject* parent);
        ~NDBCfmDlg();
        QString errString;
        QPointer<ConfirmationDialog> dlg;
        QPointer<QFrame> dlgContent;
        QPointer<QVBoxLayout> dlgContentLayout;
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
        enum result advAddCheckbox(QString const& name, QString const& label, bool checked, bool dualCol);
        enum result advAddSlider(QString const& name, QString const& label, int min, int max, int val, bool dualCol);
        enum result advAddDropDown(QString const& name, QString const& label, QStringList items, bool allowAdditionAndRemoval, bool dualCol);
    private:
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
        void connectStdSignals();
        void addWidgetToFrame(QString const& label, QWidget* widget, bool dualCol);
};

#endif // NDB_CONFIRM_DLG_H
