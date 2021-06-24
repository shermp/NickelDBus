#!/bin/sh

# Source this script from your shell or script to access these common shortcut functions

ndb_input_dlg()
{
    input_title="$1"
    [ -z "$input_title" ] && printf "Usage: ndb_input_dlg TITLE\n" && return 1
    qndb -m dlgConfirmCreate true
    qndb -m dlgConfirmSetTitle "$input_title"
    qndb -m dlgConfirmSetAccept "Ok"
    qndb -m dlgConfirmSetReject "Cancel"
    qndb -m dlgConfirmSetModal true
    qndb -s dlgConfirmTextInput -m dlgConfirmShow
    return
}

ndb_pw_dlg()
{
    pw_title="$1"
    [ -z "$pw_title" ] && printf "Usage: ndb_pw_dlg TITLE\n" && return 1
    qndb -m dlgConfirmCreate true
    qndb -m dlgConfirmSetTitle "$pw_title"
    qndb -m dlgConfirmSetAccept "Ok"
    qndb -m dlgConfirmSetReject "Cancel"
    qndb -m dlgConfirmSetModal true
    qndb -m dlgConfirmSetLEPassword true
    qndb -s dlgConfirmTextInput -m dlgConfirmShow
    return
}

ndb_modal_dlg()
{
    modal_title="$1"
    modal_body="$2"
    [ -z "$modal_title" ] || [ -z "$modal_body" ]  && printf "Usage: ndb_modal_dlg TITLE BODY\n" && return 1
    qndb -m dlgConfirmCreate
    qndb -m dlgConfirmSetTitle "$modal_title"
    qndb -m dlgConfirmSetBody "$modal_body"
    qndb -m dlgConfirmSetModal true
    qndb -m dlgConfirmShowClose false
    qndb -m dlgConfirmShow
    return
}

ndb_update_modal_dlg()
{
    modal_update="$1"
    [ -z "$modal_update" ]  && printf "Usage: ndb_update_modal_dlg BODY\n" && return 1
    qndb -m dlgConfirmSetBody "$modal_update"
    return
}

ndb_close_dlg()
{
    qndb -m dlgConfirmClose
    return
}

ndb_reboot()
{
    qndb -m pwrReboot
    return
}
