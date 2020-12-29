#ifndef NDB_CONFIRM_DLG_H
#define NDB_CONFIRM_DLG_H
#include <QObject>
#include <QDialog>
#include <QFrame>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>

typedef QDialog ConfirmationDialog;
typedef void SearchKeyboardController;
typedef int KeyboardScript;
typedef QFrame KeyboardFrame;
typedef void KeyboardReceiver;
typedef QLineEdit TouchLineEdit;
typedef QFrame TouchTextEdit;
typedef QWidget N3ConfirmationTextEditField;

class NDBCfmDlg : public QObject {
    Q_OBJECT
    public:
        enum result {Ok, NotImplemented, InitError, SymbolError, NullError, ForbiddenError};
        enum dialogType {TypeStd, TypeLineEdit};
        enum result initResult;
        NDBCfmDlg(QObject* parent, void* libnickel);
        ~NDBCfmDlg();
        QString errString;
        ConfirmationDialog* dlg;
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
    protected Q_SLOTS:
        void deactivateDialog();
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
            N3ConfirmationTextEditField *(*N3ConfirmationTextEditField__N3ConfirmationTextEditField)(
                N3ConfirmationTextEditField* _this, 
                ConfirmationDialog* dlg, 
                KeyboardScript ks);
            TouchLineEdit *(*N3ConfirmationTextEditField__textEdit)(N3ConfirmationTextEditField* _this);
        } symbols;
        bool active = false;
        bool showing = false;
        enum dialogType currActiveType;
        TouchLineEdit* tle;

        void connectStdSignals();
};

#endif // NDB_CONFIRM_DLG_H
