#include "voice.h"
#include "inlines.h"

#include <QMenu>
#include <QTimer>
#include <QPainter>
#include <math.h>
#include <qmmp/qmmp.h>

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
    connect(m_channelsAction, SIGNAL(triggered(bool)), this, SLOT(setChannelMode()));
}

Voice::~Voice()
{
    if(m_intern_vis_data)
    {
        delete[] m_intern_vis_data;
    }

    if(m_xscale)
    {
        delete[] m_xscale;
    }
}
void Voice::start()
{
    if(isVisible())
        m_timer->start();
}

void Voice::stop()
{
    m_timer->stop();
}

void Voice::typeChanged(QAction *action)
{
    switch(action->data().toInt())
    {
        case 10: m_palette = VisualPalette::PALETTE_SPECTRUM; break;
        case 20: m_palette = VisualPalette::PALETTE_SPECTROGRAM; break;
        case 30: m_palette = VisualPalette::PALETTE_SOX; break;
        case 40: m_palette = VisualPalette::PALETTE_MONO; break;
        default: break;
    }

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

void Voice::setChannelMode()
{
    initialize();
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

    const bool showTwoChannels = m_channelsAction->isChecked();

    for(int i = 1; i < m_rows; ++i)
    {
        if(m_offset >= m_cols)
        {
            m_offset = m_cols - 1;
            m_backgroundImage = m_backgroundImage.copy(1, 0, m_cols, m_rows);
        }

        if(!showTwoChannels)
        {
            const double left = qBound(0, m_intern_vis_data[i - 1] / 2, 255) / 255.0;
            m_backgroundImage.setPixel(m_offset, m_rows - i, VisualPalette::renderPalette(m_palette, left));
        }
        else
        {
            const double left = qBound(0, m_intern_vis_data[i - 1] / 2, 255) / 255.0;
            m_backgroundImage.setPixel(m_offset, (m_rows - i) / 2, VisualPalette::renderPalette(m_palette, left));

            const double right = qBound(0, m_intern_vis_data[m_rows + i - 1] / 2, 255) / 255.0;
            m_backgroundImage.setPixel(m_offset, (2 * m_rows - i) / 2, VisualPalette::renderPalette(m_palette, right));
        }
    }

    ++m_offset;
    if(!m_backgroundImage.isNull())
    {
        painter.drawImage(0, 0, m_backgroundImage);
    }
}

void Voice::contextMenuEvent(QContextMenuEvent *)
{
    QMenu menu(this);
    menu.addAction(m_channelsAction);

    QMenu typeMenu(tr("Type"), &menu);
    typeMenu.addAction(tr("Spectrum"))->setData(10);
    typeMenu.addAction(tr("Spectrogram"))->setData(20);
    typeMenu.addAction(tr("Sox"))->setData(30);
    typeMenu.addAction(tr("Mono"))->setData(40);
    connect(&typeMenu, SIGNAL(triggered(QAction*)), this, SLOT(typeChanged(QAction*)));
    menu.addMenu(&typeMenu);
    menu.exec(QCursor::pos());
}

void Voice::process(float *left, float *right)
{
    const int rows = height();
    const int cols = width();

    if(m_rows != rows || m_cols != cols)
    {
        m_rows = rows;
        m_cols = cols;

        if(m_intern_vis_data)
        {
            delete[] m_intern_vis_data;
        }

        if(m_xscale)
        {
            delete[] m_xscale;
        }

        m_intern_vis_data = new int[m_rows * 2]{0};
        m_xscale = new int[m_rows + 1]{0};

        initialize();

        for(int i = 0; i < m_rows + 1; ++i)
        {
            m_xscale[i] = pow(pow(255.0, 1.0 / m_rows), i);
        }
    }

    short dest_l[256];
    short dest_r[256];

    calc_freq(dest_l, left);
    calc_freq(dest_r, right);

    double y_scale = (double) 1.25 * m_cols / log(256);

    for(int i = 0; i < m_rows; ++i)
    {
        short yl = 0;
        short yr = 0;
        int magnitude_l = 0;
        int magnitude_r = 0;

        if(m_xscale[i] == m_xscale[i + 1])
        {
            yl = dest_l[i];
            yr = dest_r[i];
        }

        for(int k = m_xscale[i]; k < m_xscale[i + 1]; ++k)
        {
            yl = qMax(dest_l[k], yl);
            yr = qMax(dest_r[k], yr);
        }

        yl >>= 7; //256
        yr >>= 7;

        if(yl)
        {
            magnitude_l = int(log(yl) * y_scale);
            magnitude_l = qBound(0, magnitude_l, m_cols);
        }

        if(yr)
        {
            magnitude_r = int(log(yr) * y_scale);
            magnitude_r = qBound(0, magnitude_r, m_cols);
        }

        m_intern_vis_data[i] -= m_analyzerSize * m_cols / 15;
        m_intern_vis_data[i] = magnitude_l > m_intern_vis_data[i] ? magnitude_l : m_intern_vis_data[i];

        const int j = m_rows + i;
        m_intern_vis_data[j] -= m_analyzerSize * m_cols / 15;
        m_intern_vis_data[j] = magnitude_r > m_intern_vis_data[j] ? magnitude_r : m_intern_vis_data[j];
    }
}

void Voice::initialize()
{
    if(m_rows != 0 && m_cols != 0)
    {
        m_offset = 0;
        m_backgroundImage = QImage(m_cols, m_rows, QImage::Format_RGB32);
        m_backgroundImage.fill(Qt::black);
    }
}
