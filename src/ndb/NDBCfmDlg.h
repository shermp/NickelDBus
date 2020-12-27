#ifndef NDB_CONFIRM_DLG_H
#define NDB_CONFIRM_DLG_H
#include <QObject>
#include <QDialog>
#include <QFrame>
#include <QLineEdit>
#include <QTextEdit>

typedef QDialog ConfirmationDialog;
typedef void SearchKeyboardController;
typedef int KeyboardScript;
typedef QFrame KeyboardFrame;
typedef void KeyboardReceiver;
typedef QLineEdit TouchLineEdit;
typedef QFrame TouchTextEdit;

class NDBCfmDlg : public QObject {
    Q_OBJECT
    public:
        enum result {Ok, NotImplemented, InitError, SymbolError, NullError, ForbiddenError};
        enum result initResult;
        NDBCfmDlg(QObject* parent, void* libnickel);
        ~NDBCfmDlg();
        QString errString;
        ConfirmationDialog* dlg;
        enum result createDialog(
            QString const& title, 
            QString const& body, 
            QString const& acceptText, 
            QString const& rejectText, 
            bool tapOutsideClose
        );
        enum result addLineEdit();
        enum result addTextEdit();
        enum result showDialog();
        enum result closeDialog();
        enum result updateBody(QString const& body);
        QString getText();
    protected Q_SLOTS:
        void deactivateDialog();
        void detatchDialogTextLineEdit();
    private:
        enum finishedMethod {MethodNone, MethodResult, MethodLineInput};
        enum dialogType {TypeStd, TypeLineEdit, TypeTextEdit};
        struct {
            ConfirmationDialog *(*ConfirmationDialogFactory_getConfirmationDialog)(QWidget*);
            void (*ConfirmationDialog__setTitle)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setAcceptButtonText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setRejectButtonText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__showCloseButton)(ConfirmationDialog* _this, bool show);
            void (*ConfirmationDialog__setRejectOnOutsideTap)(ConfirmationDialog* _this, bool setReject);
            void (*ConfirmationDialog__addWidget)(ConfirmationDialog* _this, QWidget* addWidget);
            SearchKeyboardController *(*SearchKeyboardControllerFactory__localizedKeyboard)(QWidget*, int, QLocale const&);
            void (*SearchKeyboardController__setReceiver)(SearchKeyboardController* _this, KeyboardReceiver* receiver);
            void (*SearchKeyboardController__loadView)(SearchKeyboardController* _this);
            void (*SearchKeyboardController__setEnabled)(SearchKeyboardController* _this, bool enabled);
            KeyboardFrame *(*KeyboardFrame__KeyboardFrame)(KeyboardFrame* _this, QWidget* parent);
            SearchKeyboardController *(*KeyboardFrame_createKeyboard)(KeyboardFrame* _this, KeyboardScript script, QLocale const& loc);
            KeyboardReceiver *(*KeyboardReceiver__KeyboardReceiver_lineEdit)(KeyboardReceiver* _this, QLineEdit* line, bool dunno);
            KeyboardReceiver *(*KeyboardReceiver__KeyboardReceiver_textEdit)(KeyboardReceiver* _this, QTextEdit* text, bool dunno);
            TouchLineEdit *(*TouchLineEdit__TouchLineEdit)(TouchLineEdit* _this, QWidget* parent);
            TouchTextEdit *(*TouchTextEdit__TouchTextEdit)(TouchTextEdit* _this, QWidget* parent);
            QTextEdit *(*TouchTextEdit__textEdit)(TouchTextEdit* _this);
        } symbols;
        bool active = false;
        enum dialogType currActiveType;
        struct {
            TouchLineEdit* te;
            KeyboardReceiver* kr;
        } lineEdit;
        struct {
            TouchTextEdit* tte;
            QTextEdit* qte;
            KeyboardReceiver* kr;
        } textEdit;
};

#endif // NDB_CONFIRM_DLG_H
