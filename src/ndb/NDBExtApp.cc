#include <Qt>
#include "NDBExtApp.h"

namespace NDB {

ExtAppMode::ExtAppMode() {
    dlg.setAttribute(Qt::WA_AcceptTouchEvents);
}

enum Result ExtAppMode::enableExtApp() {
    dlg.showFullScreen();
    return Ok;
}

enum Result ExtAppMode::disableExtApp() {
    dlg.hide();
    return Ok;
}

} // NDB
