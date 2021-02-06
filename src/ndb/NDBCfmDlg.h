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

typedef QDialog ConfirmationDialog;
typedef int KeyboardScript;
typedef QLineEdit TouchLineEdit;
typedef QWidget N3ConfirmationTextEditField;
typedef QCheckBox TouchCheckBox;

class NDBCfmDlg : public QObject {
    Q_OBJECT
    public:
        enum result {Ok, NotImplemented, InitError, SymbolError, NullError, ForbiddenError};
        enum dialogType {TypeStd, TypeLineEdit};
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
        enum result addWidget(QWidget* w);
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
};

#endif // NDB_CONFIRM_DLG_H
