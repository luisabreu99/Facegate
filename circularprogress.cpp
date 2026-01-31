#include "circularprogress.h"
#include <QPainter>
#include <QPen>

CircularProgress::CircularProgress(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
}

void CircularProgress::setValue(int value)
{
    m_value = qBound(0, value, 100);
    update(); // força repaint
}

void CircularProgress::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF rect(10, 10, width() - 20, height() - 20);

    // Círculo de fundo
    QPen bgPen(QColor(255, 255, 255, 60), 6);
    p.setPen(bgPen);
    p.drawEllipse(rect);

    // Arco de progresso
    QPen progressPen(Qt::green, 6);
    p.setPen(progressPen);

    int spanAngle = static_cast<int>(360.0 * m_value / 100.0 * 16);
    p.drawArc(rect, 90 * 16, -spanAngle);
}
