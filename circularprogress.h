#ifndef CIRCULARPROGRESS_H
#define CIRCULARPROGRESS_H

#include <QWidget>

class CircularProgress : public QWidget
{
    Q_OBJECT
public:
    explicit CircularProgress(QWidget *parent = nullptr);

    void setValue(int value);   // 0 a 100

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_value = 0;   // percentagem atual
};

#endif // CIRCULARPROGRESS_H
