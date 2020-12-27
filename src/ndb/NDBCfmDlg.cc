#include <QLocale>
#include <NickelHook.h>
#include "util.h"
#include "NDBCfmDlg.h"

#define DLG_ASSERT(ret, cond, str) if (!(cond)) {           \
    errString = QString("%1: %2").arg(__func__).arg(str); \
    return (ret); }

NDBCfmDlg::NDBCfmDlg(QObject* parent, void* libnickel) : QObject(parent) {
    initResult = Ok;
    if (!libnickel) {
        initResult = InitError;
        return;
    }
    currActiveType = TypeStd;
    dlg = nullptr;
    lineEdit.te = nullptr;
    lineEdit.kr = nullptr;
    /* Resolve symbols */
    // Confirmation Dialog
    NDB_RESOLVE_SYMBOL("_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", nh_symoutptr(symbols.ConfirmationDialogFactory_getConfirmationDialog));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog8setTitleERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setTitle));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog7setTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setText));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog19setAcceptButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setAcceptButtonText));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog19setRejectButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setRejectButtonText));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog15showCloseButtonEb", nh_symoutptr(symbols.ConfirmationDialog__showCloseButton));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog21setRejectOnOutsideTapEb",nh_symoutptr(symbols.ConfirmationDialog__setRejectOnOutsideTap));
    NDB_RESOLVE_SYMBOL("_ZN18ConfirmationDialog9addWidgetEP7QWidget", nh_symoutptr(symbols.ConfirmationDialog__addWidget));
    // Keyboard stuff
    NDB_RESOLVE_SYMBOL("_ZN31SearchKeyboardControllerFactory17localizedKeyboardEP7QWidget14KeyboardScriptRK7QLocale", nh_symoutptr(symbols.SearchKeyboardControllerFactory__localizedKeyboard));
    NDB_RESOLVE_SYMBOL("_ZN24SearchKeyboardController11setReceiverEP16KeyboardReceiverb", nh_symoutptr(symbols.SearchKeyboardController__setReceiver));
    NDB_RESOLVE_SYMBOL("_ZN24SearchKeyboardController8loadViewEv", nh_symoutptr(symbols.SearchKeyboardController__loadView));
    NDB_RESOLVE_SYMBOL("_ZN24SearchKeyboardController10setEnabledEb", nh_symoutptr(symbols.SearchKeyboardController__setEnabled));
    NDB_RESOLVE_SYMBOL("_ZN13KeyboardFrameC1EP7QWidget", nh_symoutptr(symbols.KeyboardFrame__KeyboardFrame));
    NDB_RESOLVE_SYMBOL("_ZN13KeyboardFrame14createKeyboardE14KeyboardScriptRK7QLocale", nh_symoutptr(symbols.KeyboardFrame_createKeyboard));
    NDB_RESOLVE_SYMBOL("_ZN16KeyboardReceiverC1EP9QLineEditb", nh_symoutptr(symbols.KeyboardReceiver__KeyboardReceiver_lineEdit));
    NDB_RESOLVE_SYMBOL("_ZN13TouchLineEditC1EP7QWidget", nh_symoutptr(symbols.TouchLineEdit__TouchLineEdit));
}

NDBCfmDlg::~NDBCfmDlg() {
}

enum NDBCfmDlg::result NDBCfmDlg::createDialog(
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
        symbols.ConfirmationDialog__setTitle &&
        symbols.ConfirmationDialog__setText && 
        symbols.ConfirmationDialog__setAcceptButtonText && 
        symbols.ConfirmationDialog__setRejectButtonText && 
        symbols.ConfirmationDialog__setRejectOnOutsideTap,
        "could not find one or more standard dialog symbols"
    );
    dlg = symbols.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
    DLG_ASSERT(NullError, dlg, "could not get confirmation dialog");
    active = true;
    currActiveType = TypeStd;
    symbols.ConfirmationDialog__setTitle(dlg, title);
    symbols.ConfirmationDialog__setText(dlg, body);
    symbols.ConfirmationDialog__setRejectOnOutsideTap(dlg, tapOutsideClose);

    if (!acceptText.isEmpty()) { symbols.ConfirmationDialog__setAcceptButtonText(dlg, acceptText); }
    if (!rejectText.isEmpty()) { symbols.ConfirmationDialog__setRejectButtonText(dlg, rejectText); }

    dlg->setModal(true);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::showDialog() {
    if (currActiveType == TypeLineEdit || currActiveType == TypeTextEdit) {
        DLG_ASSERT(SymbolError, symbols.KeyboardFrame_createKeyboard && symbols.SearchKeyboardController__setReceiver, "could not find one or more symbols")
        KeyboardReceiver *kbrc = lineEdit.kr;
        QLocale loc;
        KeyboardFrame* kbf = dlg->findChild<KeyboardFrame*>(QString("keyboardFrame"));
        DLG_ASSERT(NullError, kbf, "could not find KeyboardFrame");
        SearchKeyboardController *skc = symbols.KeyboardFrame_createKeyboard(kbf, 0, loc);
        DLG_ASSERT(NullError, skc, "could not get SearchKeyboardController");
        symbols.SearchKeyboardController__setReceiver(skc, kbrc);
        kbf->show();
    }
    QObject::connect(dlg, &QDialog::finished, this, &NDBCfmDlg::deactivateDialog);
    QObject::connect(dlg, &QDialog::finished, this, &NDBCfmDlg::detatchDialogTextLineEdit);
    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    dlg->open();
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::closeDialog() {
    DLG_ASSERT(ForbiddenError, active, "dialog not open");
    dlg->accept();
    return Ok;
}

void NDBCfmDlg::deactivateDialog() {
    active = false;
}

void NDBCfmDlg::detatchDialogTextLineEdit() {
    if (currActiveType == TypeLineEdit) {
        lineEdit.te->setParent(nullptr); 
        currActiveType = TypeStd;
    }
}

enum NDBCfmDlg::result NDBCfmDlg::updateBody(QString const& body) {
    DLG_ASSERT(ForbiddenError, active, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeStd, "not standard dialog");
    DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__setText, "could not find setText symbol");
    symbols.ConfirmationDialog__setText(dlg, body);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::addLineEdit() {
    DLG_ASSERT(ForbiddenError, active, "dialog not active");
    DLG_ASSERT(
        SymbolError, 
        symbols.TouchLineEdit__TouchLineEdit &&
        symbols.KeyboardReceiver__KeyboardReceiver_lineEdit &&
        symbols.ConfirmationDialog__addWidget,
        "could not find symbols"
    );
    if (!lineEdit.te) { 
        lineEdit.te = (QLineEdit*)calloc(1, sizeof(QLineEdit) * 2);
        DLG_ASSERT(NullError, lineEdit.te, "could not allocate TouchLineEdit");
        symbols.TouchLineEdit__TouchLineEdit(lineEdit.te, nullptr);
    }
    if (!lineEdit.kr) { 
        lineEdit.kr = calloc(1, sizeof(QFrame) * 2);
        DLG_ASSERT(NullError, lineEdit.kr, "could not allocate KeyboardReceiver for TouchLineEdit");
        symbols.KeyboardReceiver__KeyboardReceiver_lineEdit(lineEdit.kr, lineEdit.te, true);
    }
    lineEdit.te->clear();
    symbols.ConfirmationDialog__addWidget(dlg, lineEdit.te);
    currActiveType = TypeLineEdit;
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::addTextEdit() {
    return NotImplemented;
}

QString NDBCfmDlg::getText() {
    DLG_ASSERT(QString(""), active, "dialog not active");
    if (currActiveType == TypeLineEdit) {
        return lineEdit.te->text();
    }
    return QString("");
}