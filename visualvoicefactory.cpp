#include "visualvoicefactory.h"
#include "voice.h"

#include <QMessageBox>

VisualProperties VisualVoiceFactory::properties() const
{
    VisualProperties properties;
    properties.name = tr("Voice Plugin");
    properties.shortName = "voice";
    properties.hasAbout = true;
    return properties;
}

Visual *VisualVoiceFactory::create(QWidget *parent)
{
    return new Voice(parent);
}

QDialog *VisualVoiceFactory::createConfigDialog(QWidget *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

void VisualVoiceFactory::showAbout(QWidget *parent)
{
    QMessageBox::about (parent, tr("About Voice Visual Plugin"),
                        tr("Qmmp Voice Visual Plugin")+"\n"+
                        tr("This plugin adds Voice visualization")+"\n"+
                        tr("Written by: Greedysky <greedysky@163.com>"));
}

QString VisualVoiceFactory::translation() const
{
    return QString();
}
