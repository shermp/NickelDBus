#include <Qt>
#include <QLocale>
#include <QRegularExpression>
#include <QList>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
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
    ndbResolveSymbolRTLD("_ZNK18ConfirmationDialog13keyboardFrameEv", nh_symoutptr(symbols.ConfirmationDialog__keyboardFrame));

    styleSheet = QString(R"(
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
    if (dlgType == TypeLineEdit || dlgType == TypeAdvanced) {
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
        dlg = symbols.ConfirmationDialogFactory_showTextEditDialog(title);
        DLG_ASSERT(NullError, dlg, "could not get line edit dialog");
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
    
    case TypeAdvanced:
        DLG_ASSERT(SymbolError, symbols.ConfirmationDialog__setContent, "could not find setContent() symbol");
        dlg = symbols.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
        DLG_ASSERT(NullError, dlg, "could not get confirmation dialog");
        advContent = new QFrame;
        advMainLayout = new QVBoxLayout;
        advActiveLayout = nullptr;
        advContent->setLayout(advMainLayout);

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
        // Set the final layout
        if (advActiveLayout)
            advMainLayout->addLayout(advActiveLayout);

        advContent->setStyleSheet(styleSheet);
        symbols.ConfirmationDialog__setContent(dlg, advContent);
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

void NDBCfmDlg::addWidgetToFrame(QString const& label, QWidget* widget) {
    using namespace NDBTouchWidgets;
    // If the user hasn't set a layout, default to vertical box
    if (!advActiveLayout) {
        advActiveLayout = new QVBoxLayout;
    }
    auto cb = qobject_cast<TouchCheckBox*>(widget);
    auto vbl = qobject_cast<QVBoxLayout*>(advActiveLayout);
    auto fl = qobject_cast<QFormLayout*>(advActiveLayout);
    auto hbl = qobject_cast<QHBoxLayout*>(advActiveLayout);
    if (vbl && cb) {
        cb->setText(label);
        vbl->addWidget(cb);
        return;
    }
    TouchLabel *lbl = NDBTouchLabel::create(label, nullptr, 0);
    if (vbl || hbl) {
        advActiveLayout->addWidget(lbl);
        advActiveLayout->addWidget(widget);
    } else if (fl) {
        fl->addRow(lbl, widget);
    } else {
        nh_log("both form and vertical layouts were null");
    }
    return;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddLayout(enum NDBCfmDlg::layoutType lt) {
    if (advActiveLayout) {
        advMainLayout->addLayout(advActiveLayout);
    }
    switch (lt) {
    case HorLayout:
        advActiveLayout = new QHBoxLayout;
        break;
    case VertLayout:
        advActiveLayout = new QVBoxLayout;
        break;
    case FormLayout:
        advActiveLayout = new QFormLayout;
        break;
    default:
        advActiveLayout = nullptr;
    }
    DLG_ASSERT(NullError, advActiveLayout, "unabled to get new layout");
    return Ok;
}

#define DLG_SET_OBJ_NAME(obj, name) (obj)->setObjectName(QString("ndb_%1").arg(name))

enum NDBCfmDlg::result NDBCfmDlg::advAddCheckbox(QString const& name, QString const& label, bool checked) {
    using namespace NDBTouchWidgets;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    TouchCheckBox *cb = NDBTouchCheckBox::create(nullptr);
    DLG_ASSERT(NullError, cb, "unable to create checkbox");
    DLG_SET_OBJ_NAME(cb, name);
    Qt::CheckState cs = (checked) ? Qt::Checked : Qt::Unchecked;
    cb->setCheckState(cs);
    addWidgetToFrame(label, cb);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddSlider(QString const& name, QString const& label, int min, int max, int val) {
    using namespace NDBTouchWidgets;
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
    addWidgetToFrame(label, f);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddDropDown(QString const& name, QString const& label, QStringList items, bool allowAdditionAndRemoval __attribute__((unused))) {
    using namespace NDBTouchWidgets;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    TouchDropDown *td = NDBTouchDropDown::create(nullptr, true);
    DLG_ASSERT(NullError, td, "unable to create TouchDropDown");
    DLG_SET_OBJ_NAME(td, name);
    for (int i = 0; i < items.size(); ++i) {
        NDBTouchDropDown::addItem(td, items[i], QVariant(items[i]), false);
    }
    NDBTouchDropDown::setCurrentIndex(td, 0);
    addWidgetToFrame(label, td);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddLineEdit(QString const& name, QString const& label) {
    using namespace NDBTouchWidgets::NDBKeyboard;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    auto tle = createLineEdit(dlg);
    DLG_ASSERT(NullError, tle, "unable to create TextLineEdit");
    DLG_ASSERT(ConnError, 
                QObject::connect(tle, SIGNAL(tapped()), this, SLOT(onLineTextEditTapped())), 
                "unable to connect TouchLineEdit::tapped() to onLineTextEditTapped()");
    DLG_SET_OBJ_NAME(tle, name);
    addWidgetToFrame(label, tle);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddTextEdit(QString const& name, QString const& label) {
    using namespace NDBTouchWidgets::NDBKeyboard;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    auto tte = createTextEdit(dlg);
    DLG_ASSERT(NullError, tte, "unable to create TouchTextEdit");
    DLG_ASSERT(ConnError, 
                QObject::connect(tte, SIGNAL(tapped()), this, SLOT(onLineTextEditTapped())), 
                "unable to connect TouchTextEdit::tapped() to onLineTextEditTapped()");
    DLG_SET_OBJ_NAME(tte, name);
    addWidgetToFrame(label, tte);
    return Ok;
}

void NDBCfmDlg::onLineTextEditTapped() {
    using namespace NDBTouchWidgets::NDBKeyboard;
    auto sender = QObject::sender();
    auto tle = qobject_cast<TouchLineEdit*>(sender);
    auto tte = qobject_cast<TouchTextEdit*>(sender);
    if (!tle && !tte)
        nh_log("onLineTextEditTapped: sender not TouchLineEdit or TouchTextEdit");
        return;
    
    auto kr = getKeyboardReciever(qobject_cast<QWidget*>(sender));
    if (!kr)
        nh_log("onLineTextEditTapped: can't get keyboard receiver");
        return;
    
    auto kf = symbols.ConfirmationDialog__keyboardFrame(dlg);
    if (!kf)
        nh_log("onLineTextEditTapped: can't get keyboard frame");
        return;
    
    auto skbc = createKeyboard(kf, 1, QLocale());
    setReceiver(skbc, kr);
    if (tte)
        setMultilineEntry(skbc, true);

    QObject::connect(skbc, SIGNAL(commitRequested()), this, SLOT(onCommitRequested()));
    kf->show();
}

void NDBCfmDlg::onCommitRequested() {
    auto kf = symbols.ConfirmationDialog__keyboardFrame(dlg);
    if (!kf)
        return;
    kf->hide();
}

enum NDBCfmDlg::result NDBCfmDlg::advAddDatePicker(QString const& name, QString const& label, QDate init) {
    using namespace NDBTouchWidgets::NDBDateTime;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    auto d = create(dlg, init);
    DLG_ASSERT(NullError, d, "unable to create N3DatePicker");
    DLG_SET_OBJ_NAME(d, name);
    addWidgetToFrame(label, d);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advAddTimePicker(QString const& name, QString const& label, QTime init) {
    using namespace NDBTouchWidgets::NDBDateTime;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    auto t = create(dlg, init);
    DLG_ASSERT(NullError, t, "unable to create N3TimePicker");
    DLG_SET_OBJ_NAME(t, name);
    addWidgetToFrame(label, t);
    return Ok;
}

enum NDBCfmDlg::result NDBCfmDlg::advGetJSON(QString& json) {
    using namespace NDBTouchWidgets;
    DLG_ASSERT(ForbiddenError, dlg, "dialog must exist");
    // Get all widgets in the frame whose name begins with 'ndb_'
    QRegularExpression re("ndb_.+");
    QList<QWidget*> widgets = advContent->findChildren<QWidget*>(re);
    //nh_log("advGetJSON: Found %d widgets", widgets.size());
    QVariantMap qvm;
    for (int i = 0; i < widgets.size(); ++i) {
        QString className = widgets[i]->metaObject()->className();
        QString name = widgets[i]->objectName().remove(QRegularExpression("ndb_"));
        //nh_log("advGetJSON: Widget %d has className '%s' and name '%s'", i, className.toUtf8().constData(), name.toUtf8().constData());
        if (className == "TouchCheckBox") {
            TouchCheckBox *cb = qobject_cast<TouchCheckBox*>(widgets[i]);
            qvm[name] = QVariant(cb->isChecked());
        } else if (className == "TouchSlider") {
            TouchSlider *sl = qobject_cast<TouchSlider*>(widgets[i]);
            qvm[name] = QVariant(sl->value());
        } else if (className == "TouchDropDown" || className == "BlockTouchDropDown") {
            TouchDropDown *dd = qobject_cast<TouchDropDown*>(widgets[i]);
            qvm[name] = NDBTouchDropDown::currentData(dd);
        } else if (className == "TouchLineEdit") {
            TouchLineEdit *t = qobject_cast<TouchLineEdit*>(widgets[i]);
            DLG_ASSERT(NullError, t, "unable to get TouchLineEdit");
            qvm[name] = QVariant(t->text());
        } else if (className == "TouchTextEdit") {
            TouchTextEdit *t = qobject_cast<TouchTextEdit*>(widgets[i]);
            DLG_ASSERT(NullError, t, "unable to get TouchTextEdit");
            qvm[name] = QVariant(NDBKeyboard::textEdit(t)->toPlainText());
        } else if (className == "N3DatePicker") {
            N3DatePicker *d = qobject_cast<N3DatePicker*>(widgets[i]);
            qvm[name] = QVariant(NDBDateTime::getDate(d));
        } else if (className == "N3TimePicker") {
            N3TimePicker *t = qobject_cast<N3TimePicker*>(widgets[i]);
            qvm[name] = QVariant(NDBDateTime::getTime(t));
        }
    }
    QJsonObject obj = QJsonObject::fromVariantMap(qvm);
    //nh_log("QJsonObject is %s", (obj.isEmpty() ? "empty" : "not empty"));
    QJsonDocument doc(obj);
    //nh_log("QJsonDocument is %s", (doc.isNull() ? "invalid" : "valid"));
    QByteArray docBA = doc.toJson(QJsonDocument::Compact);
    //nh_log("JSON contents is: %s", docBA.constData());
    json.append(QString().fromUtf8(docBA));
    return Ok;
}

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
