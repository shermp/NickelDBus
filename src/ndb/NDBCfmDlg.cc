#include <Qt>
#include <QLocale>
#include <NickelHook.h>
#include "util.h"
#include "NDBCfmDlg.h"

#define DLG_ASSERT(ret, cond, str) if (!(cond)) {         \
    errString = QString("%1: %2").arg(__func__).arg(str); \
    if (dlg) {                                            \
        connectStdSignals();                              \
        closeDialog();                                    \
    }                                                     \
    return (ret);                                         \
}

namespace NDB {

NDBCfmDlg::NDBCfmDlg(QObject* parent) : QObject(parent) {
    initResult = Ok;
    currActiveType = TypeStd;
    dlgStyleSheet = QString(R"(
            * {
                font-family: Avenir, sans-serif;
                font-style: normal;
                padding: 0px;
                margin: 0px;
            }
            *[localeName="ja"] {
                font-family: Sans-SerifJP, sans-serif;
                font-style: normal;
            }
            *[localeName="zh"] {
                font-family: Sans-SerifZH-Simplified, sans-serif;
                font-style: normal;
            }
            *[localeName="zh-HK"] {
                font-family: Sans-SerifZH-Traditional, sans-serif;
                font-style: normal;
            }
            *[localeName="zh-TW"] {
                font-family: Sans-SerifZH-Traditional, sans-serif;
                font-style: normal;
            }

            *[qApp_deviceIsTrilogy=true] {
                font-size: 23px;
            }
            *[qApp_deviceIsPhoenix=true] {
                font-size: 26px;
            }
            *[qApp_deviceIsDragon=true] {
                font-size: 32px;
            }
            *[qApp_deviceIsAlyssum=true] {
                font-size: 35px;
            }
            *[qApp_deviceIsNova=true] {
                font-size: 35px;
            }
            *[qApp_deviceIsStorm=true] {
                font-size: 44px;
            }
            *[qApp_deviceIsDaylight=true] {
                font-size: 42px;
            }
        )");
    
    /* Resolve symbols */
    // Confirmation Dialog
    resolveSymbolRTLD("_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", nh_symoutptr(symbols.ConfirmationDialogFactory_getConfirmationDialog));
    resolveSymbolRTLD("_ZN25ConfirmationDialogFactory18showTextEditDialogERK7QString", nh_symoutptr(symbols.ConfirmationDialogFactory_showTextEditDialog));
    resolveSymbolRTLD("_ZN18ConfirmationDialog8setTitleERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setTitle));
    resolveSymbolRTLD("_ZN18ConfirmationDialog7setTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setText));
    resolveSymbolRTLD("_ZN18ConfirmationDialog19setAcceptButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setAcceptButtonText));
    resolveSymbolRTLD("_ZN18ConfirmationDialog19setRejectButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setRejectButtonText));
    resolveSymbolRTLD("_ZN18ConfirmationDialog15showCloseButtonEb", nh_symoutptr(symbols.ConfirmationDialog__showCloseButton));
    resolveSymbolRTLD("_ZN18ConfirmationDialog21setRejectOnOutsideTapEb", nh_symoutptr(symbols.ConfirmationDialog__setRejectOnOutsideTap));
    resolveSymbolRTLD("_ZN18ConfirmationDialog9addWidgetEP7QWidget", nh_symoutptr(symbols.ConfirmationDialog__addWidget));

    // Keyboard stuff
    resolveSymbolRTLD("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog14KeyboardScript", nh_symoutptr(symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS));
    if (!symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS) {
        // FW 4.6 has a slightly different constructor without the KeyboardScript stuff
        resolveSymbolRTLD("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog", nh_symoutptr(symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField));
    }
    resolveSymbolRTLD("_ZNK27N3ConfirmationTextEditField8textEditEv", nh_symoutptr(symbols.N3ConfirmationTextEditField__textEdit));
}

NDBCfmDlg::~NDBCfmDlg() {
}

void NDBCfmDlg::connectStdSignals() {
    if (dlg) {
        NDB_DEBUG("connecting standard signals");
        // Connecting accept/reject signals instead of finished, because for some
        // reason the dialog created by 'showTextEditDialog()' connects the accept
        // button tap directly to the 'accepted' signal, instead of the 'accept' slot.
        // If Kobo ever changes this behaviour, the following code should still work.
        QObject::connect(dlg, &QDialog::accepted, dlg, &QDialog::deleteLater, Qt::UniqueConnection);
        QObject::connect(dlg, &QDialog::rejected, dlg, &QDialog::deleteLater, Qt::UniqueConnection);
    }
}

N3ConfirmationTextEditField* NDBCfmDlg::createTextEditField() {
    N3ConfirmationTextEditField *t = reinterpret_cast<N3ConfirmationTextEditField*>(calloc(1,128));
    if (!t || !dlg) {return nullptr;}
    if (symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS) {
        symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS(t, dlg, 1);
    } else {
        symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField(t, dlg);
    }
    return t;
}

enum Result NDBCfmDlg::createDialog(enum dialogType dlgType) {
    DLG_ASSERT(ForbiddenError, !dlg, "dialog already open");
    DLG_ASSERT(
        SymbolError, 
        symbols.ConfirmationDialogFactory_getConfirmationDialog && 
        symbols.ConfirmationDialogFactory_showTextEditDialog &&
        symbols.ConfirmationDialog__setTitle &&
        symbols.ConfirmationDialog__setText && 
        symbols.ConfirmationDialog__setAcceptButtonText && 
        symbols.ConfirmationDialog__setRejectButtonText && 
        symbols.ConfirmationDialog__setRejectOnOutsideTap && 
        symbols.ConfirmationDialog__showCloseButton,
        "could not find one or more standard dialog symbols"
    );
    if (dlgType == TypeLineEdit) {
        DLG_ASSERT(
            SymbolError,
            (symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS ||
             symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField) &&
            symbols.N3ConfirmationTextEditField__textEdit,
            "could not find text edit symbols"
        );
    }

    switch (dlgType) {
    case TypeStd:
        dlg = symbols.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
        DLG_ASSERT(NullError, dlg, "could not get confirmation dialog");
        currActiveType = TypeStd;
        break;

    case TypeLineEdit:
        dlg = symbols.ConfirmationDialogFactory_showTextEditDialog("");
        DLG_ASSERT(NullError, dlg, "could not get line edit dialog");
        dlg->hide();
        tef = createTextEditField();
        DLG_ASSERT(NullError, tef, "error getting text edit field");
        tle = symbols.N3ConfirmationTextEditField__textEdit(tef);
        DLG_ASSERT(NullError, tle, "error getting TouchLineEdit");
        // Make the 'Go' key accept the dialog.
        if (!QObject::connect(tef, SIGNAL(commitRequested()), dlg, SIGNAL(accepted()))) {
            nh_log("unable to connect N3ConfirmationTextEditField::commitRequested() to ConfirmationDialog::accepted()");
        }
        currActiveType = TypeLineEdit;
        break;

    default:
        DLG_ASSERT(ParamError, false, "Incorrect dialog type passed");
        break;
    }

    dlg->setModal(true);
    return Ok;
}

enum Result NDBCfmDlg::showDialog() {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    connectStdSignals();
    dlg->open();
    return Ok;
}

enum Result NDBCfmDlg::closeDialog() {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    dlg->accept();
    return Ok;
}

enum Result NDBCfmDlg::setTitle(QString const& title) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    symbols.ConfirmationDialog__setTitle(dlg, title);
    return Ok;
}

enum Result NDBCfmDlg::setBody(QString const& body) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeStd, "not standard dialog");
    symbols.ConfirmationDialog__setText(dlg, body);
    return Ok;
}

enum Result NDBCfmDlg::setAccept(QString const& acceptText) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    symbols.ConfirmationDialog__setAcceptButtonText(dlg, acceptText);
    return Ok;
}

enum Result NDBCfmDlg::setReject(QString const& rejectText) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    symbols.ConfirmationDialog__setRejectButtonText(dlg, rejectText);
    return Ok;
}

enum Result NDBCfmDlg::setModal(bool modal) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    symbols.ConfirmationDialog__setRejectOnOutsideTap(dlg, !modal);
    return Ok;
}

enum Result NDBCfmDlg::showClose(bool show) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    symbols.ConfirmationDialog__showCloseButton(dlg, show);
    return Ok;
}

enum Result NDBCfmDlg::setProgress(int min, int max, int val, QString const& format) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeStd, "not standard dialog");
    DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__addWidget, "could not find addWidget symbol");
    bool added = true;
    if (min < 0 || max < 0 || val < 0) {
        if (prog) {
            prog->hide();
        }
        return Ok;
    } else if (prog) {
        prog->show();
    }
    if (!prog) {
        prog = new NDBProgressBar();
        prog->setStyleSheet(dlgStyleSheet);
        added = false;
    }
    prog->setMinimum(min);
    prog->setMaximum(max);
    prog->setValue(val);
    if (!format.isEmpty()) {
        prog->setFormat(format);
    }
    if (!added) {
        symbols.ConfirmationDialog__addWidget(dlg, prog);
    }
    return Ok;
}

enum Result NDBCfmDlg::setLEPassword(bool isPassword) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeLineEdit, "not LineEdit dialog");
    DLG_ASSERT(NullError, tef, "TextEditField is null");

    TouchCheckBox* tcb = tef->findChild<TouchCheckBox*>(QString("showPassword"));
    if (tcb) {
        if (isPassword) {
            tcb->setChecked(false);
            tcb->show();
        }
        else {
            tcb->setChecked(true);
            tcb->hide();
        }
    }
    return Ok;
}
enum Result NDBCfmDlg::setLEPlaceholder(QString const& placeholder) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeLineEdit, "not LineEdit dialog");
    DLG_ASSERT(NullError, tle, "TouchLineEdit is null");
    tle->setPlaceholderText(placeholder);
    return Ok;
}

QString NDBCfmDlg::getLEText() {
    QString res;
    DLG_ASSERT(res, dlg, "dialog not open");
    DLG_ASSERT(res, currActiveType == TypeLineEdit, "not LineEdit dialog");
    DLG_ASSERT(res, tle, "TouchLineEdit is null");
    return tle->text();
}

} // namespace NDB

/*
 * A note on my current understanding of how the keyboard stuff works:
 * 
 * Start with a TouchLineEdit (TLE) or TouchTextEdit (TTE). TouchLineEdit is a subclass
 * of QLineEdit, TouchLineEdit is a subclass of a QFrame, which contains a
 * QTextEdit.
 * 
 * To attach the keyboard to the above widgets, a KeyboardReceiver (KR) is used, the 
 * constructor for this adds a KR as a child object of the TLE or 
 * TTE. 
 * 
 * Both the edit widgets and KR need to be manually heap allocated before use. Care
 * needs to be taken that enough memory is allocated.
 * 
 * The keyboard itself is contained in a KeyboardFrame (KF). The ConfirmationDialog 
 * already contains a KF, and has a method to get a pointer to it (ConfirmationDialog::keyboardFrame()).
 * The KF has a method to create the actual keyboard (KeyboardFrame::createKeyboard()), 
 * which is locale dependent. One of the parameters required for createKeyboard() is a 
 * KeyboardScript, which is unknown at this time, but is likely an enum.
 * 
 * createKeyboard() returns (a pointer to) a SearchKeyboardController (SKC), whic is a 
 * descendant of the KeyboardController (KC) class. To finally connect the edit widget 
 * to the keyboard, SearchKeyboardController::setReceiver() is used to set the previously 
 * created KR to the SKC.
 * 
 * To actually show and hide the keyboard, we can use QWidget::show() and QWidget::hide() on the 
 * KF. The edit widgets have a tapped() signal used to show the keyboard, and the KC has a 
 * commitRequested() signal to hide the keyboard when the 'go' key is pressed.
*/
