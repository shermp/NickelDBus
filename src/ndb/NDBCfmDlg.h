#ifndef NDB_CONFIRM_DLG_H
#define NDB_CONFIRM_DLG_H
#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QWidget>
#include <QPointer>
#include <QProgressBar>
#include <QCheckBox>
#include "NDBWidgets.h"
#include "ndb.h"

typedef QDialog ConfirmationDialog;
typedef int KeyboardScript;
typedef QLineEdit TouchLineEdit;
typedef QWidget N3ConfirmationTextEditField;
typedef QCheckBox TouchCheckBox;

namespace NDB {

class NDBCfmDlg : public QObject {
    Q_OBJECT
    public:
        enum dialogType {TypeStd, TypeLineEdit};
        enum Result initResult;
        NDBCfmDlg(QObject* parent);
        ~NDBCfmDlg();
        QString errString;
        QPointer<ConfirmationDialog> dlg;
        enum Result createDialog(enum dialogType dlgType);
        enum Result setTitle(const QString& title);
        enum Result setBody(QString const& body);
        enum Result setAccept(QString const& acceptText);
        enum Result setReject(QString const& rejectText);
        enum Result setModal(bool modal);
        enum Result showClose(bool show);
        enum Result setProgress(int min, int max, int val, QString const& format = "");
        enum Result setLEPassword(bool isPassword);
        enum Result setLEPlaceholder(QString const& placeholder);
        QString getLEText();
        enum Result showDialog();
        enum Result closeDialog();

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
        QString dlgStyleSheet;
        QPointer<NDBProgressBar> prog;
        QPointer<TouchLineEdit> tle;
        QPointer<N3ConfirmationTextEditField> tef;
        N3ConfirmationTextEditField* createTextEditField();
        void connectStdSignals();

};

} // namespace NDB

#endif // NDB_CONFIRM_DLG_H
