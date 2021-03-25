#include <QTimer>
#include <QPainter>
#include <QPaintEvent>
#include <QMenu>
#include <math.h>
#include <stdlib.h>

#include "inlines.h"
#include "voice.h"

Voice::Voice(QWidget *parent)
    : Visual(parent)
{
    setWindowTitle(tr("Voice Widget"));
    setMinimumSize(2 * 300 - 30, 105);

    m_timer = new QTimer(this);
    m_timer->setInterval(40);
    connect(m_timer, SIGNAL(timeout()), SLOT(updateVisual()));

    m_screenAction = new QAction(tr("Fullscreen"), this);
    m_screenAction->setCheckable(true);
    connect(m_screenAction, SIGNAL(triggered(bool)), this, SLOT(setFullScreen(bool)));
}

Voice::~Voice()
{
    if(m_intern_vis_data)
    {
        delete[] m_intern_vis_data;
    }

    if(m_x_scale)
    {
        delete[] m_x_scale;
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

void Voice::setFullScreen(bool yes)
{
    if(yes)
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void Voice::hideEvent(QHideEvent *)
{
    m_timer->stop();
}

void Voice::showEvent(QShowEvent *)
{
    m_timer->start();
}

void Voice::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.fillRect(e->rect(), Qt::black);
    draw(&painter);
}

void Voice::contextMenuEvent(QContextMenuEvent *e)
{
    Visual::contextMenuEvent(e);
    QMenu menu(this);
    QMenu typeMenu(tr("Type"), &menu);
    typeMenu.addAction(tr("Spectrum"))->setData(10);
    typeMenu.addAction(tr("Spectrogram"))->setData(20);
    typeMenu.addAction(tr("Sox"))->setData(30);
    typeMenu.addAction(tr("Mono"))->setData(40);
    connect(&typeMenu, SIGNAL(triggered(QAction*)), this, SLOT(typeChanged(QAction*)));
    menu.addMenu(&typeMenu);
    menu.exec(QCursor::pos());
}

void Voice::process(float *left, float *)
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

        if(m_x_scale)
        {
            delete[] m_x_scale;
        }

        m_intern_vis_data = new int[m_rows]{0};
        m_x_scale = new int[m_rows + 1]{0};

        initialize();

        for(int i = 0; i < m_rows + 1; ++i)
        {
            m_x_scale[i] = pow(pow(255.0, 1.0 / m_rows), i);
        }
    }

    short dest[256];
    short y;
    int k, magnitude;

    calc_freq(dest, left);
    double y_scale = (double) 1.25 * m_cols / log(256);

    for(int i = 0; i < m_rows; i++)
    {
        y = 0;
        magnitude = 0;

        if(m_x_scale[i] == m_x_scale[i + 1])
        {
            y = dest[i];
        }

        for(k = m_x_scale[i]; k < m_x_scale[i + 1]; k++)
        {
            y = qMax(dest[k], y);
        }

        y >>= 7; //256

        if(y)
        {
            magnitude = int(log(y) * y_scale);
            magnitude = qBound(0, magnitude, m_cols);
        }

        m_intern_vis_data[i] -= m_analyzer_falloff * m_cols / 15;
        m_intern_vis_data[i] = magnitude > m_intern_vis_data[i] ? magnitude : m_intern_vis_data[i];
    }
}

void Voice::draw(QPainter *p)
{
    p->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    for(int j = 1; j < m_rows; ++j)
    {
        if(m_pixPos >= m_cols)
        {
            m_pixPos = m_cols - 1;
            m_backgroundImage = m_backgroundImage.copy(1, 0, m_cols, m_rows);
        }

        const double level = qBound(0, m_intern_vis_data[j - 1] / 2, 255) / 255.0;
        m_backgroundImage.setPixel(m_pixPos, m_rows - j, VisualPalette::renderPalette(m_palette, level));
    }

    ++m_pixPos;
    if(!m_backgroundImage.isNull())
    {
        p->drawImage(0, 0, m_backgroundImage);
    }
}

void Voice::initialize()
{
    if(m_rows != 0 && m_cols != 0)
    {
        m_pixPos = 0;
        m_backgroundImage = QImage(m_cols, m_rows, QImage::Format_RGB32);
        m_backgroundImage.fill(Qt::black);
    }
}
