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

#if (QMMP_VERSION_INT < 0x10700) || (0x20000 <= QMMP_VERSION_INT && QMMP_VERSION_INT < 0x20200)
QDialog *VisualVoiceFactory::createConfigDialog(QWidget *parent)
#else
QDialog *VisualVoiceFactory::createSettings(QWidget *parent)
#endif
{
    Q_UNUSED(parent);
    return nullptr;
}

void VisualVoiceFactory::showAbout(QWidget *parent)
{
    QMessageBox::about(parent, tr("About Voice Visual Plugin"),
                       tr("Qmmp Voice Visual Plugin") + "\n" +
                       tr("Written by: Greedysky <greedysky@163.com>"));
}

QString VisualVoiceFactory::translation() const
{
    return QString();
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
#include <QtPlugin>
Q_EXPORT_PLUGIN2(voice, VisualVoiceFactory)
#endif
