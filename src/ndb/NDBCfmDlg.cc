#include <QLocale>
#include <NickelHook.h>
#include "util.h"
#include "NDBCfmDlg.h"

#define DLG_ASSERT(ret, cond, str) if (!(cond)) {         \
    errString = QString("%1: %2").arg(__func__).arg(str); \
    if (active && !showing) {                             \
        connectStdSignals();                              \
        closeDialog();                                    \
    }                                                     \
    return (ret);                                         \
}

NDBCfmDlg::NDBCfmDlg(QObject* parent, void* libnickel) : QObject(parent) {
    initResult = Ok;
    if (!libnickel) {
        initResult = InitError;
        return;
    }
    currActiveType = TypeStd;
    dlg = nullptr;
    tle = nullptr;
    /* Resolve symbols */
    // Confirmation Dialog
    NDB_RESOLVE_SYMBOL("_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", nh_symoutptr(symbols.ConfirmationDialogFactory_getConfirmationDialog));
    NDB_RESOLVE_SYMBOL("_ZN25ConfirmationDialogFactory18showTextEditDialogERK7QString", nh_symoutptr(symbols.ConfirmationDialogFactory_showTextEditDialog));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog8setTitleERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setTitle));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog7setTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setText));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog19setAcceptButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setAcceptButtonText));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog19setRejectButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setRejectButtonText));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog15showCloseButtonEb", nh_symoutptr(symbols.ConfirmationDialog__showCloseButton));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog21setRejectOnOutsideTapEb",nh_symoutptr(symbols.ConfirmationDialog__setRejectOnOutsideTap));
    // Keyboard stuff
    NDB_RESOLVE_SYMBOL("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog14KeyboardScript", nh_symoutptr(symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField));
    NDB_RESOLVE_SYMBOL("_ZNK27N3ConfirmationTextEditField8textEditEv", nh_symoutptr(symbols.N3ConfirmationTextEditField__textEdit));
}

NDBCfmDlg::~NDBCfmDlg() {
}

void NDBCfmDlg::connectStdSignals() {
    if (active) {
        NDB_DEBUG("connecting standard signals");
        // Connecting accept/reject signals instead of finished, because for some
        // reason the dialog created by 'showTextEditDialog()' connects the accept
        // button tap directly to the 'accepted' signal, instead of the 'accept' slot.
        // If Kobo ever changes this behaviour, the following code should still work.
        QObject::connect(dlg, &QDialog::accepted, this, &NDBCfmDlg::deactivateDialog);
        QObject::connect(dlg, &QDialog::rejected, this, &NDBCfmDlg::deactivateDialog);
        QObject::connect(dlg, &QDialog::accepted, dlg, &QDialog::deleteLater);
        QObject::connect(dlg, &QDialog::rejected, dlg, &QDialog::deleteLater);
    }
}

enum NDBCfmDlg::result NDBCfmDlg::createDialog(
    enum dialogType dlgType,
    QString const& title, 
    QString const& body, 
    QString const& acceptText, 
    QString const& rejectText, 
    bool tapOutsideClose
) {
    DLG_ASSERT(ForbiddenError, !active, "dialog already open");
    DLG_ASSERT(
        SymbolError, 
        symbols.ConfirmationDialogFactory_getConfirmationDialog && 
        symbols.ConfirmationDialogFactory_showTextEditDialog &&
        symbols.ConfirmationDialog__setTitle &&
        symbols.ConfirmationDialog__setText && 
        symbols.ConfirmationDialog__setAcceptButtonText && 
        symbols.ConfirmationDialog__setRejectButtonText && 
        symbols.ConfirmationDialog__setRejectOnOutsideTap,
        "could not find one or more standard dialog symbols"
    );
    if (dlgType == TypeLineEdit) {
        dlg = symbols.ConfirmationDialogFactory_showTextEditDialog(title);
        DLG_ASSERT(NullError, dlg, "could not get line edit dialog");
        DLG_ASSERT(
            SymbolError,
            symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField &&
            symbols.N3ConfirmationTextEditField__textEdit,
            "could not find symbols"
        );
        // Use ::operator new() instead of malloc/calloc so that Qt can call delete later
        N3ConfirmationTextEditField* tef = reinterpret_cast<N3ConfirmationTextEditField*>(::operator new(128));
        DLG_ASSERT(NullError, tef, "error getting text edit field");
        memset(tef, 0, 128);
        // Still don't know what 'KeyboardScript' is, but I've seen code in libnickel that uses 1 so...
        symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField(tef, dlg, 1);
        tle = symbols.N3ConfirmationTextEditField__textEdit(tef);
        DLG_ASSERT(NullError, tle, "error getting TouchLineEdit");
        currActiveType = TypeLineEdit;
    } else {
        dlg = symbols.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
        DLG_ASSERT(NullError, dlg, "could not get confirmation dialog");
        currActiveType = TypeStd;
    }
    active = true;
    symbols.ConfirmationDialog__setTitle(dlg, title);
    symbols.ConfirmationDialog__setText(dlg, body);
    symbols.ConfirmationDialog__setRejectOnOutsideTap(dlg, tapOutsideClose);

    if (!acceptText.isEmpty()) { symbols.ConfirmationDialog__setAcceptButtonText(dlg, acceptText); }
    if (!rejectText.isEmpty()) { symbols.ConfirmationDialog__setRejectButtonText(dlg, rejectText); }

    dlg->setModal(true);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::showDialog() {
    connectStdSignals();
    dlg->open();
    showing = true;
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::closeDialog() {
    DLG_ASSERT(ForbiddenError, active, "dialog not open");
    dlg->accept();
    return Ok;
}

void NDBCfmDlg::deactivateDialog() {
    NDB_DEBUG("deactivating dialog");
    active = false;
    showing = false;
    tle = nullptr;
}

enum NDBCfmDlg::result NDBCfmDlg::updateBody(QString const& body) {
    DLG_ASSERT(ForbiddenError, active, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeStd, "not standard dialog");
    DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__setText, "could not find setText symbol");
    symbols.ConfirmationDialog__setText(dlg, body);
    return Ok;
}

void NDBCfmDlg::setPassword(bool isPassword) {
    if (active && currActiveType == TypeLineEdit && tle) {
        if (isPassword) {
            tle->setEchoMode(QLineEdit::Password);
        } else {
            tle->setEchoMode(QLineEdit::Normal);
        }
    }
}

QString NDBCfmDlg::getText() {
    QString ret("");
    DLG_ASSERT(ret, active, "dialog not active");
    if (currActiveType == TypeLineEdit && tle) {
        ret = tle->text();
    }
    return ret;
}

void NDBCfmDlg::setText(QString const& text) {
    if (active && currActiveType == TypeLineEdit && tle) {
        tle->setPlaceholderText(text);
    }
}