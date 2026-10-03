#include "VueImage.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

VueImage::VueImage(QWidget* parent) : QWidget(parent) {
  setMinimumSize(200, 200);
  setAutoFillBackground(true);
  QPalette palette = this->palette();
  palette.setColor(QPalette::Window, Qt::black);
  setPalette(palette);
}

void VueImage::definirImage(const QImage& image, const QString& legende) {
  image_ = image;
  legende_ = legende;
  update();
}

void VueImage::definirMinuties(const std::vector<nis::Sommet>& minuties) {
  minuties_ = minuties;
  update();
}

void VueImage::effacer() {
  image_ = QImage();
  legende_.clear();
  minuties_.clear();
  update();
}

QSize VueImage::sizeHint() const { return QSize(320, 320); }

void VueImage::paintEvent(QPaintEvent* evenement) {
  QWidget::paintEvent(evenement);
  QPainter peintre(this);
  peintre.fillRect(rect(), Qt::black);
  if (image_.isNull()) {
    peintre.setPen(Qt::gray);
    peintre.drawText(rect(), Qt::AlignCenter,
                     legende_.isEmpty() ? QString::fromUtf8("Aucune image") : legende_);
    return;
  }
  // Cadre utile : image conservee en ratio, centree.
  const QSize cadre = size() - QSize(8, legende_.isEmpty() ? 8 : 30);
  const QSize ajuste = image_.size().scaled(cadre, Qt::KeepAspectRatio);
  const QPoint origine((width() - ajuste.width()) / 2, 4);
  const QRect cible(origine, ajuste);
  peintre.drawImage(cible, image_);
  const double echelle =
      static_cast<double>(ajuste.width()) / static_cast<double>(image_.width());
  for (const nis::Sommet& s : minuties_) {
    const QPointF centre(origine.x() + s.x * echelle, origine.y() + s.y * echelle);
    if (s.type == nis::TypeMinutie::Terminaison) {
      peintre.setPen(QPen(Qt::red, 2));
      peintre.setBrush(Qt::NoBrush);
      peintre.drawEllipse(centre, 5.0, 5.0);
    } else {
      peintre.setPen(QPen(Qt::blue, 2));
      peintre.setBrush(Qt::NoBrush);
      peintre.drawRect(QRectF(centre.x() - 5.0, centre.y() - 5.0, 10.0, 10.0));
    }
  }
  if (!legende_.isEmpty()) {
    peintre.setPen(Qt::white);
    peintre.drawText(QRect(4, height() - 24, width() - 8, 20), Qt::AlignCenter, legende_);
  }
}
