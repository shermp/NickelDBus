#include <Qt>
#include <QLocale>
#include <NickelHook.h>
#include "util.h"
#include "NDBCfmDlg.h"
#include "NDBTouchWidgets.h"

#define DLG_ASSERT(ret, cond, str) if (!(cond)) {         \
    errString = QString("%1: %2").arg(__func__).arg(str); \
    if (dlg) {                                            \
        connectStdSignals();                              \
        closeDialog();                                    \
    }                                                     \
    return (ret);                                         \
}

NDBCfmDlg::NDBCfmDlg(QObject* parent) : QObject(parent) {
    initResult = Ok;
    currActiveType = TypeStd;
    /* Resolve symbols */
    // Confirmation Dialog
    ndbResolveSymbolRTLD("_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", nh_symoutptr(symbols.ConfirmationDialogFactory_getConfirmationDialog));
    ndbResolveSymbolRTLD("_ZN25ConfirmationDialogFactory18showTextEditDialogERK7QString", nh_symoutptr(symbols.ConfirmationDialogFactory_showTextEditDialog));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog8setTitleERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setTitle));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog7setTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setText));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog19setAcceptButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setAcceptButtonText));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog19setRejectButtonTextERK7QString", nh_symoutptr(symbols.ConfirmationDialog__setRejectButtonText));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog15showCloseButtonEb", nh_symoutptr(symbols.ConfirmationDialog__showCloseButton));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog21setRejectOnOutsideTapEb", nh_symoutptr(symbols.ConfirmationDialog__setRejectOnOutsideTap));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog9addWidgetEP7QWidget", nh_symoutptr(symbols.ConfirmationDialog__addWidget));
    ndbResolveSymbolRTLD("_ZN18ConfirmationDialog10setContentEP7QWidget", nh_symoutptr(symbols.ConfirmationDialog__setContent));
    // Keyboard stuff
    ndbResolveSymbolRTLD("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog14KeyboardScript", nh_symoutptr(symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS));
    if (!symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS) {
        // FW 4.6 has a slightly different constructor without the KeyboardScript stuff
        ndbResolveSymbolRTLD("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog", nh_symoutptr(symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField));
    }
    ndbResolveSymbolRTLD("_ZNK27N3ConfirmationTextEditField8textEditEv", nh_symoutptr(symbols.N3ConfirmationTextEditField__textEdit));
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

enum NDBCfmDlg::result NDBCfmDlg::createDialog(
    enum dialogType dlgType,
    QString const& title, 
    QString const& body, 
    QString const& acceptText, 
    QString const& rejectText, 
    bool tapOutsideClose
) {
    DLG_ASSERT(ForbiddenError, !dlg, "dialog already open");
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
    switch (dlgType) {
    case TypeStd:
        dlg = symbols.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
        DLG_ASSERT(NullError, dlg, "could not get confirmation dialog");
        currActiveType = TypeStd;
        break;

    case TypeLineEdit:
        dlg = symbols.ConfirmationDialogFactory_showTextEditDialog(title);
        DLG_ASSERT(NullError, dlg, "could not get line edit dialog");
        DLG_ASSERT(
            SymbolError,
            (symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS ||
             symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditField) &&
            symbols.N3ConfirmationTextEditField__textEdit,
            "could not find symbols"
        );
        if (symbols.N3ConfirmationTextEditField__N3ConfirmationTextEditFieldKS) {
            tef = ndbCreateNickelObject<N3ConfirmationTextEditField>("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog14KeyboardScript", 128, dlg, 1);
        } else {
            tef = ndbCreateNickelObject<N3ConfirmationTextEditField>("_ZN27N3ConfirmationTextEditFieldC1EP18ConfirmationDialog", 128, dlg);
        }
        DLG_ASSERT(NullError, tef, "error getting text edit field");
        tle = symbols.N3ConfirmationTextEditField__textEdit(tef);
        DLG_ASSERT(NullError, tle, "error getting TouchLineEdit");
        // Make the 'Go' key accept the dialog.
        if (!QObject::connect(tef, SIGNAL(commitRequested()), dlg, SIGNAL(accepted()))) {
            nh_log("unable to connect N3ConfirmationTextEditField::commitRequested() to ConfirmationDialog::accepted()");
        }
        currActiveType = TypeLineEdit;
        break;
    
    case TypeAdvanced:
        DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__setContent, "could not find setContent() symbol");
        dlg = symbols.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
        DLG_ASSERT(NullError, dlg, "could not get confirmation dialog");
        dlgContent = new QFrame;
        dlgContentLayout = new QVBoxLayout;
        dlgContent->setLayout(dlgContentLayout);
        currActiveType = TypeAdvanced;
        break;

    default:
        DLG_ASSERT(ParamError, false, "Incorrect dialog type passed");
        break;
    }

    symbols.ConfirmationDialog__setTitle(dlg, title);
    symbols.ConfirmationDialog__setText(dlg, body);
    symbols.ConfirmationDialog__setRejectOnOutsideTap(dlg, tapOutsideClose);

    if (!acceptText.isEmpty()) { symbols.ConfirmationDialog__setAcceptButtonText(dlg, acceptText); }
    if (!rejectText.isEmpty()) { symbols.ConfirmationDialog__setRejectButtonText(dlg, rejectText); }

    dlg->setModal(true);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::showDialog() {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    switch (currActiveType) {
    case TypeAdvanced:
        symbols.ConfirmationDialog__setContent(dlg, dlgContent);
        break;
    default:
        break;
    }
    connectStdSignals();
    dlg->open();
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::closeDialog() {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    dlg->accept();
    return Ok;
}

// enum NDBCfmDlg::result NDBCfmDlg::addWidget(QWidget* w) {
//     DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
//     DLG_ASSERT(ForbiddenError, currActiveType == TypeStd, "not standard dialog");
//     DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__addWidget, "could not find addWidget symbol");
//     symbols.ConfirmationDialog__addWidget(dlg, w);
//     return Ok;
// }

enum NDBCfmDlg::result NDBCfmDlg::updateBody(QString const& body) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog not open");
    DLG_ASSERT(ForbiddenError, currActiveType == TypeStd, "not standard dialog");
    DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__setText, "could not find setText symbol");
    symbols.ConfirmationDialog__setText(dlg, body);
    return Ok;
}

void NDBCfmDlg::setPassword(bool isPassword) {
    if (dlg && currActiveType == TypeLineEdit && tef) {
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
    }
}

QString NDBCfmDlg::getText() {
    QString ret;
    DLG_ASSERT(ret, dlg, "dialog not active");
    if (currActiveType == TypeLineEdit && tle) {
        ret = tle->text();
    }
    return ret;
}

void NDBCfmDlg::setText(QString const& text) {
    if (dlg && currActiveType == TypeLineEdit && tle) {
        tle->setPlaceholderText(text);
    }
}

void NDBCfmDlg::addWidgetToFrame(QString const& label, QWidget* widget, bool dualCol) {
    QFrame *f = new QFrame;
    QGridLayout *gl = new QGridLayout(f);
    TouchLabel *lbl = NDBTouchLabel::create(label, nullptr, 0);
    int wRow = (dualCol) ? 0 : 1;
    int wCol = (dualCol) ? 1 : 0;
    enum Qt::AlignmentFlag wAlign = (dualCol) ? Qt::AlignRight : Qt::AlignLeft; 
    gl->addWidget(lbl, 0, 0, 0);
    gl->addWidget(widget, wRow, wCol, wAlign);
    gl->setColumnStretch(0, 2);
    gl->setColumnStretch(1, 0);
    dlgContentLayout->addWidget(f);
}

#define DLG_SET_OBJ_NAME(obj, name) (obj)->setObjectName(QString("ndb_%1").arg(name))

enum NDBCfmDlg::result NDBCfmDlg::advAddCheckbox(QString const& name, QString const& label, bool checked, bool dualCol) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    TouchCheckBox *cb = NDBTouchCheckBox::create(nullptr);
    DLG_ASSERT(NullError, cb, "unable to create checkbox");
    DLG_SET_OBJ_NAME(cb, name);
    Qt::CheckState cs = (checked) ? Qt::Checked : Qt::Unchecked;
    cb->setCheckState(cs);
    if (dualCol) {
        addWidgetToFrame(label, cb, dualCol);
    } else {
        cb->setText(label);
        dlgContentLayout->addWidget(cb);
    }
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddSlider(QString const& name, QString const& label, int min, int max, int val, bool dualCol) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    TouchSlider *sl = NDBTouchSlider::create(nullptr);
    DLG_ASSERT(NullError, sl, "unable to create slider");
    DLG_SET_OBJ_NAME(sl, name);
    TouchLabel *minLabel = NDBTouchLabel::create(QString::number(min), nullptr, 0);
    DLG_ASSERT(NullError, minLabel, "unable to create min label");
    TouchLabel *maxLabel = NDBTouchLabel::create(QString::number(max), nullptr, 0);
    DLG_ASSERT(NullError, maxLabel, "unable to create max label");
    sl->setMaximum(max);
    sl->setMinimum(min);
    sl->setValue(val);
    sl->setOrientation(Qt::Horizontal);
    QFrame *f = new QFrame;
    QGridLayout *slLayout = new QGridLayout(f);
    slLayout->addWidget(minLabel, 0, 0);
    slLayout->addWidget(sl, 0, 1, Qt::AlignCenter);
    slLayout->addWidget(maxLabel, 0, 2, Qt::AlignRight);
    slLayout->setColumnStretch(1, 2);
    addWidgetToFrame(label, f, dualCol);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddDropDown(QString const& name, QString const& label, QStringList items, bool allowAdditionAndRemoval __attribute__((unused)), bool dualCol) {
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    TouchDropDown *td = NDBTouchDropDown::create(nullptr, true);
    DLG_ASSERT(NullError, td, "unable to create TouchDropDown");
    DLG_SET_OBJ_NAME(td, name);
    for (int i = 0; i < items.size(); ++i) {
        NDBTouchDropDown::addItem(td, items[i], QVariant(items[i]), false);
    }
    NDBTouchDropDown::setCurrentIndex(td, 0);
    addWidgetToFrame(label, td, dualCol);
    return Ok;
}
