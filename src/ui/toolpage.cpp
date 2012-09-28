/*
Copyright (C) 2011-2012 Yubico AB.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "toolpage.h"
#include "ui_toolpage.h"
#include "ui/helpbox.h"

#include "common.h"

ToolPage::ToolPage(QWidget *parent) :
        QStackedWidget(parent),
        ui(new Ui::ToolPage)
{
    ui->setupUi(this);

    //Connect pages
    connectPages();

    //Connect help buttons
    connectHelpButtons();

    //Connect other signals and slots
    connect(ui->converterResetBtn, SIGNAL(clicked()),
            this, SLOT(resetConverterPage()));
    connect(ui->chalRespResetBtn, SIGNAL(clicked()),
            this, SLOT(resetChalRespPage()));
    connect(ui->chalRespPerformBtn, SIGNAL(clicked()),
            this, SLOT(performChallengeResponse()));
    connect(ui->chalRespChallenge, SIGNAL(editingFinished()),
            this, SLOT(on_chalRespChallenge_editingFinished()));
    connect(ui->ndefResetBtn, SIGNAL(clicked()),
            this, SLOT(resetNdefPage()));
    connect(ui->ndefProgramBtn, SIGNAL(clicked()),
            this, SLOT(programNdef()));

    connect(YubiKeyFinder::getInstance(), SIGNAL(keyFound(bool, bool*)),
            this, SLOT(keyFound(bool, bool*)));
}

ToolPage::~ToolPage() {
    delete ui;
}

/*
 Common
*/

void ToolPage::connectPages() {
    //Map the values of the navigation buttons with the indexes of
    //the stacked widget

    //Create a QMapper
    QSignalMapper *mapper = new QSignalMapper(this);

    //Connect the clicked signal with the QSignalMapper
    connect(ui->converterBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->converterBackBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    connect(ui->chalRespBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->chalRespBackBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    connect(ui->ndefBtn, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->ndefBackBtn, SIGNAL(clicked()), mapper, SLOT(map()));

    //Set a value for each button
    mapper->setMapping(ui->converterBtn, Page_Converter);
    mapper->setMapping(ui->converterBackBtn, Page_Base);

    mapper->setMapping(ui->chalRespBtn, Page_ChalResp);
    mapper->setMapping(ui->chalRespBackBtn, Page_Base);

    mapper->setMapping(ui->ndefBtn, Page_Ndef);
    mapper->setMapping(ui->ndefBackBtn, Page_Base);

    //Connect the mapper to the widget
    //The mapper will set a value to each button and
    //set that value to the widget
    //connect(pageMapper, SIGNAL(mapped(int)), this, SLOT(setCurrentIndex(int)));
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(setCurrentPage(int)));

    //Set the current page
    m_currentPage = 0;
    setCurrentIndex(Page_Base);
}

void ToolPage::setCurrentPage(int pageIndex) {
    //Page changed...

    m_currentPage = pageIndex;

    switch(pageIndex){
    case Page_Converter:
        resetConverterPage();
        break;
    }

    setCurrentIndex(pageIndex);
}

void ToolPage::connectHelpButtons() {
}

void ToolPage::helpBtn_pressed(int helpIndex) {
    HelpBox help(this);
    help.setHelpIndex((HelpBox::Help)helpIndex);
    help.exec();
}

void ToolPage::resetChalRespPage() {
    ui->chalRespChallenge->clear();
    ui->chalRespResponse->clear();
}

void ToolPage::on_chalRespChallenge_editingFinished() {
    QString challenge = ui->chalRespChallenge->text().trimmed();
    ui->chalRespChallenge->setText(challenge);
}

void ToolPage::performChallengeResponse() {
    QString challenge = ui->chalRespChallenge->text();
    QString response = "";
    bool hmac;
    int slot;
    if(ui->chalRespHmacRadio->isChecked()) {
        hmac = true;
    } else if(ui->chalRespYubicoRadio->isChecked()) {
        hmac = false;
    } else {
      emit showStatusMessage(ERR_CHAL_TYPE_NOT_SELECTED, 1);
      return;
    }
    if(ui->chalRespSlot1Radio->isChecked()) {
        slot = 1;
    } else if(ui->chalRespSlot2Radio->isChecked()) {
        slot = 2;
    } else {
      emit showStatusMessage(ERR_CONF_SLOT_NOT_SELECTED, 1);
      return;
    }
    YubiKeyWriter::getInstance()->doChallengeResponse(challenge, response, slot, hmac);
    qDebug() << "response was: " << response;
    ui->chalRespResponse->setText(response);
}
/*
 Quick Page handling
*/
void ToolPage::resetConverterPage() {
    convert(0, "");
    ui->converterHexTxt->setCursorPosition(0);
    ui->converterHexTxt->setFocus();
}

void ToolPage::convert(int updatedIndex, QString txt) {
    unsigned char buf[16];
    memset(buf, 0, sizeof(buf));
    size_t bufLen = 0;

    switch(updatedIndex) {
    case 0: //Hex
        YubiKeyUtil::qstrHexDecode(buf, &bufLen, txt);
        break;

    case 1: //Modhex
        YubiKeyUtil::qstrModhexDecode(buf, &bufLen, txt);
        break;

    case 2: //Decimal
        QString tmp = QString::number(txt.toULongLong(), 16);
        size_t len = tmp.length();
        if(len % 2 != 0) {
            len++;
        }
        YubiKeyUtil::qstrClean(&tmp, (size_t)len, true);
        YubiKeyUtil::qstrHexDecode(buf, &bufLen, tmp);
        break;
    }

    QString hex = YubiKeyUtil::qstrHexEncode(buf, bufLen);
    QString modhex = YubiKeyUtil::qstrModhexEncode(buf, bufLen);
    bool ok = false;
    qulonglong dec = hex.toULongLong(&ok, 16);

    int hexLen = hex.length();
    int modhexLen = modhex.length();

    ui->converterHexTxt->setText(hex);
    ui->converterModhexTxt->setText(modhex);
    ui->converterDecTxt->setText(QString::number(dec));

    ui->converterHexCopyBtn->setEnabled(hexLen > 0);
    ui->converterModhexCopyBtn->setEnabled(modhexLen > 0);
    ui->converterDecCopyBtn->setEnabled(
            ui->converterDecTxt->text().length() > 0);

    ui->converterHexLenLbl->setText(tr("(%1 chars)").arg(hexLen));
    ui->converterModhexLenLbl->setText(tr("(%1 chars)").arg(modhexLen));

    if(hexLen != 0 && !ok) {
        ui->converterDecErrLbl->setText(TOVERFLOW);
    } else {
        ui->converterDecErrLbl->setText(tr(""));
    }
}

void ToolPage::on_converterHexTxt_editingFinished() {
    QString txt = ui->converterHexTxt->text();
    YubiKeyUtil::qstrClean(&txt, 0, true);

    size_t len = txt.length();
    if(len > 0) {
        if(len % 2 != 0) {
            len++;
        }
        YubiKeyUtil::qstrClean(&txt, (size_t)len, true);
        convert(0, txt);
    }
    ui->converterHexTxt->setCursorPosition(len + len/2);
}

void ToolPage::on_converterModhexTxt_editingFinished() {
    QString txt = ui->converterModhexTxt->text();
    YubiKeyUtil::qstrModhexClean(&txt, 0, true);

    size_t len = txt.length();
    if(len > 0) {
        if(len % 2 != 0) {
            len++;
        }
        YubiKeyUtil::qstrModhexClean(&txt, (size_t)len, true);
        convert(1, txt);
    }
    ui->converterModhexTxt->setCursorPosition(len + len/2);
}

void ToolPage::on_converterDecTxt_editingFinished() {
    QString txt = ui->converterDecTxt->text();
    bool ok = false;
    qulonglong dec = txt.toULongLong(&ok);
    if(ok) {
        if(dec > 0) {
            size_t len = txt.length();
            YubiKeyUtil::qstrClean(&txt, (size_t)len, true);
            convert(2, txt);
            ui->converterDecTxt->setCursorPosition(len);
        }
    } else {
        ui->converterDecErrLbl->setText(TOVERFLOW);
    }

}

void ToolPage::copyToClipboard(const QString &str) {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str);

    showStatusMessage(VALUE_COPIED, 0);
}

void ToolPage::on_converterHexCopyBtn_clicked() {
    QString txt = ui->converterHexTxt->text();
    YubiKeyUtil::qstrClean(&txt, 0, true);

    copyToClipboard(txt);
}

void ToolPage::on_converterModhexCopyBtn_clicked() {
    QString txt = ui->converterModhexTxt->text();
    YubiKeyUtil::qstrModhexClean(&txt, 0, true);

    copyToClipboard(txt);
}

void ToolPage::on_converterDecCopyBtn_clicked() {
    copyToClipboard(ui->converterDecTxt->text());
}

void ToolPage::resetNdefPage() {
    ui->ndefEdit->clear();
    ui->ndefTextLangEdit->setText("en-US");
    ui->ndefUriRadio->setChecked(true);
}

void ToolPage::programNdef() {
    YubiKeyWriter *writer = YubiKeyWriter::getInstance();
    bool uri = true;
    QString language;
    QString payload;
    if(ui->ndefTextRadio->isChecked()) {
        uri = false;
        language = ui->ndefTextLangEdit->text().trimmed();
        if(language.isEmpty()) {
            return;
        }
    }
    payload = ui->ndefEdit->text().trimmed();
    if(payload.isEmpty()) {
        return;
    }

    connect(writer, SIGNAL(configWritten(bool, const QString &)),
            this, SLOT(ndefWritten(bool, const QString &)));
    writer->writeNdef(uri, language, payload);
}

void ToolPage::ndefWritten(bool written, const QString &msg) {
    disconnect(YubiKeyWriter::getInstance(), SIGNAL(configWritten(bool, const QString &)),
            this, SLOT(ndefWritten(bool, const QString &)));
    if(written) {
        showStatusMessage(tr("NDEF successfully written"));
    }
}

void ToolPage::on_ndefTextRadio_toggled(bool checked) {
    if(checked) {
        ui->ndefTextLangEdit->setEnabled(true);
    } else {
        ui->ndefTextLangEdit->setText("en-US");
        ui->ndefTextLangEdit->setEnabled(false);
    }
}

void ToolPage::keyFound(bool found, bool* featuresMatrix) {
    if(found && featuresMatrix[YubiKeyFinder::Feature_ChallengeResponse]) {
        ui->chalRespPerformBtn->setEnabled(true);
    } else {
        ui->chalRespPerformBtn->setEnabled(false);
    }
    if(found && featuresMatrix[YubiKeyFinder::Feature_Ndef]) {
        ui->ndefProgramBtn->setEnabled(true);
    } else {
        ui->ndefProgramBtn->setEnabled(false);
    }
}
