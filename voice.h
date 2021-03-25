/* =================================================
 * This file is part of the TTK qmmp plugin project
 * Copyright (C) 2015 - 2021 Greedysky Studio

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; If not, see <http://www.gnu.org/licenses/>.
 ================================================= */

#ifndef VOICE_H
#define VOICE_H

#include <qmmp/visual.h>
#include "visualpalette.h"

/*!
 * @author Greedysky <greedysky@163.com>
 */
class Voice : public Visual
{
    Q_OBJECT
public:
    explicit Voice(QWidget *parent = nullptr);
    virtual ~Voice();

public slots:
    virtual void start() override;
    virtual void stop() override;

private slots:
    void typeChanged(QAction *action);
    void updateVisual();
    void setFullScreen(bool yes);

private:
    virtual void hideEvent(QHideEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;
    virtual void paintEvent(QPaintEvent *) override;
    virtual void contextMenuEvent(QContextMenuEvent *e) override;

    void process(float *left, float *right);
    void draw(QPainter *p);
    void initialize();

    VisualPalette::Palette m_palette= VisualPalette::PALETTE_DEFAULT;
    QImage m_backgroundImage;
    int m_pixPos = 0;
    int *m_x_scale = nullptr;
    double m_analyzer_falloff = 2.2;
    QTimer *m_timer = nullptr;
    int m_rows = 0, m_cols = 0;
    int *m_intern_vis_data = nullptr;
    float m_left[QMMP_VISUAL_NODE_SIZE];
    float m_right[QMMP_VISUAL_NODE_SIZE];
    QAction *m_screenAction = nullptr;

};

#endif
