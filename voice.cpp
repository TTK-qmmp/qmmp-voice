#include "voice.h"
#include "inlines.h"

#include <QMenu>
#include <QTimer>
#include <QPainter>
#include <QSettings>
#include <QActionGroup>
#include <math.h>
#include <qmmp/qmmp.h>

#define MIN_ROW     270
#define MIN_COLUMN  300

Voice::Voice(QWidget *parent)
    : Visual(parent)
{
    setWindowTitle(tr("Voice Widget"));
    setMinimumSize(2 * 300 - 30, 105);

    m_timer = new QTimer(this);
    m_timer->setInterval(40);
    connect(m_timer, SIGNAL(timeout()), SLOT(updateVisual()));

    m_channelsAction = new QAction(tr("Double Channels"), this);
    m_channelsAction->setCheckable(true);
    connect(m_channelsAction, SIGNAL(triggered(bool)), this, SLOT(writeSettings()));

    createPalette(MIN_ROW);
    createMenu();
    readSettings();
}

Voice::~Voice()
{
    delete[] m_visualData;
    delete[] m_xscale;
}

void Voice::start()
{
    if(isVisible())
    {
        m_timer->start();
    }
}

void Voice::stop()
{
    m_timer->stop();
}

void Voice::readSettings()
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QSettings settings;
#else
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
#endif
    settings.beginGroup("Voice");
    m_channelsAction->setChecked(settings.value("show_two_channels", true).toBool());
    m_palette = static_cast<VisualPalette::Palette>(settings.value("palette", VisualPalette::PALETTE_DEFAULT).toInt());
    m_rangeValue = settings.value("range", 30).toInt();
    settings.endGroup();

    for(QAction *act : m_typeActions->actions())
    {
        if(m_palette == static_cast<VisualPalette::Palette>(act->data().toInt()))
        {
            act->setChecked(true);
            break;
        }
    }

    for(QAction *act : m_rangeActions->actions())
    {
        if(m_rangeValue == act->data().toInt())
        {
            act->setChecked(true);
            break;
        }
    }
}

void Voice::writeSettings()
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QSettings settings;
#else
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
#endif
    settings.beginGroup("Voice");
    settings.setValue("show_two_channels", m_channelsAction->isChecked());
    QAction *act = m_typeActions->checkedAction();
    settings.setValue("palette", m_palette = (act ? static_cast<VisualPalette::Palette>(act->data().toInt()) : VisualPalette::PALETTE_DEFAULT));
    act = m_rangeActions->checkedAction();
    settings.setValue("range", m_rangeValue = (act ? act->data().toInt() : 30));
    settings.endGroup();

    initialize();
}

void Voice::updateVisual()
{
    if(takeData(m_left, m_right))
    {
        process(m_left, m_right);
        update();
    }
}

void Voice::hideEvent(QHideEvent *)
{
    m_timer->stop();
}

void Voice::showEvent(QShowEvent *)
{
    m_timer->start();
}

void Voice::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if(m_backgroundImage.isNull())
    {
        return;
    }

    const bool showTwoChannels = m_channelsAction->isChecked();
    const int level = 255 - m_rangeValue;

    for(int i = 1; i < m_rows; ++i)
    {
        const int w = m_backgroundImage.width();
        if(m_offset >= w)
        {
            m_offset = w - 1;
            m_backgroundImage = m_backgroundImage.copy(1, 0, w, m_backgroundImage.height());
        }

        if(!showTwoChannels)
        {
            const double left = qBound(0, m_visualData[i - 1] / 2, level) * 1.0 / level;
            m_backgroundImage.setPixel(m_offset, m_rows - i, VisualPalette::renderPalette(m_palette, left));
        }
        else
        {
            const double left = qBound(0, m_visualData[i - 1] / 2, level) * 1.0 / level;
            m_backgroundImage.setPixel(m_offset, m_rows - i, VisualPalette::renderPalette(m_palette, left));

            const double right = qBound(0, m_visualData[m_rows + i - 1] / 2, level) * 1.0 / level;
            m_backgroundImage.setPixel(m_offset, 2 * m_rows - i, VisualPalette::renderPalette(m_palette, right));
        }
    }

    ++m_offset;
    painter.drawImage(0, (height() - (showTwoChannels ? 2 : 1) * m_rows) / 2, m_backgroundImage);
}

void Voice::contextMenuEvent(QContextMenuEvent *)
{
    m_menu->exec(QCursor::pos());
}

void Voice::process(float *left, float *right)
{
    const int rows = height();
    const int cols = width();

    if(rows < 2 * MIN_ROW && m_rows != rows / 2)
    {
        createPalette(rows / 2);
        initialize();
    }
    else if(rows >= 2 * MIN_ROW && m_rows != MIN_ROW)
    {
        createPalette(MIN_ROW);
        initialize();
    }
    else if(m_backgroundImage.width() != cols)
    {
        initialize();
    }

    short destl[256], destr[256];
    calc_freq(destl, left);
    calc_freq(destr, right);

    const double yscale = (double)1.25 * m_cols / log(256);

    for(int i = 0; i < m_rows; ++i)
    {
        short yl = 0, yr = 0;
        int magnitudel = 0, magnituder = 0;

        if(m_xscale[i] == m_xscale[i + 1])
        {
            yl = (i >= 256 ? 0 : (destl[i] >> 7)); //128
            yr = (i >= 256 ? 0 : (destr[i] >> 7)); //128
        }

        for(int k = m_xscale[i]; k < m_xscale[i + 1]; ++k)
        {
            yl = (k >= 256 ? 0 : qMax(short(destl[k] >> 7), yl));
            yr = (k >= 256 ? 0 : qMax(short(destr[k] >> 7), yr));
        }

        if(yl > 0)
        {
            magnitudel = qBound(0, int(log(yl) * yscale), m_cols);
        }

        if(yr > 0)
        {
            magnituder = qBound(0, int(log(yr) * yscale), m_cols);
        }

        m_visualData[i] -= m_analyzerSize * m_cols / 15;
        m_visualData[i] = magnitudel > m_visualData[i] ? magnitudel : m_visualData[i];

        const int j = m_rows + i;
        m_visualData[j] -= m_analyzerSize * m_cols / 15;
        m_visualData[j] = magnituder > m_visualData[j] ? magnituder : m_visualData[j];
    }
}

void Voice::createMenu()
{
    m_menu = new QMenu(this);
    connect(m_menu, SIGNAL(triggered(QAction*)), SLOT(writeSettings()));

    m_menu->addAction(m_channelsAction);

    m_typeActions = new QActionGroup(this);
    m_typeActions->setExclusive(true);
    m_typeActions->addAction(tr("Spectrum"))->setData(0);
    m_typeActions->addAction(tr("Perceptual"))->setData(1);
    m_typeActions->addAction(tr("Rainbow"))->setData(2);
    m_typeActions->addAction(tr("Sox"))->setData(3);
    m_typeActions->addAction(tr("Magma"))->setData(4);
    m_typeActions->addAction(tr("Linas"))->setData(5);
    m_typeActions->addAction(tr("CubeHelix"))->setData(6);
    m_typeActions->addAction(tr("Fractalizer"))->setData(7);
    m_typeActions->addAction(tr("Mono"))->setData(8);

    QMenu *typeMenu = m_menu->addMenu(tr("Type"));
    for(QAction *act : m_typeActions->actions())
    {
        act->setCheckable(true);
        typeMenu->addAction(act);
    }

    m_rangeActions = new QActionGroup(this);
    m_rangeActions->setExclusive(true);
    m_rangeActions->addAction(tr("0 DB"))->setData(0);
    m_rangeActions->addAction(tr("10 DB"))->setData(10);
    m_rangeActions->addAction(tr("20 DB"))->setData(20);
    m_rangeActions->addAction(tr("30 DB"))->setData(30);
    m_rangeActions->addAction(tr("40 DB"))->setData(40);
    m_rangeActions->addAction(tr("50 DB"))->setData(50);
    m_rangeActions->addAction(tr("60 DB"))->setData(60);
    m_rangeActions->addAction(tr("70 DB"))->setData(70);
    m_rangeActions->addAction(tr("80 DB"))->setData(80);
    m_rangeActions->addAction(tr("90 DB"))->setData(90);
    m_rangeActions->addAction(tr("100 DB"))->setData(100);
    m_rangeActions->addAction(tr("110 DB"))->setData(110);
    m_rangeActions->addAction(tr("120 DB"))->setData(120);

    QMenu *rangeMenu = m_menu->addMenu(tr("Range"));
    for(QAction *act : m_rangeActions->actions())
    {
        act->setCheckable(true);
        rangeMenu->addAction(act);
    }
}

void Voice::createPalette(int row)
{
    m_rows = row;
    m_cols = MIN_COLUMN;

    delete[] m_visualData;
    delete[] m_xscale;

    m_visualData = new int[m_rows * 2]{0};
    m_xscale = new int[m_rows + 1]{0};

    for(int i = 0; i < m_rows + 1; ++i)
    {
        m_xscale[i] = pow(255.0, float(i) / m_rows);
    }
}

void Voice::initialize()
{
    m_offset = 0;
    m_backgroundImage = QImage(width(), (m_channelsAction->isChecked() ? 2 : 1) * m_rows, QImage::Format_RGB32);
    m_backgroundImage.fill(Qt::black);
}
