// Zone d'affichage d'une image et de ses minuties.
//
// Remplace les statics SS_BLACKFRAME/SS_BITMAP de `CimgDlg` : l'image est
// redimensionnee en conservant son rapport (pas de StretchDraw ecrase) et les
// minuties sont dessinees par-dessus (disques rouges = terminaisons, carres
// bleus = bifurcations).
#ifndef NIS_QT_VUE_IMAGE_H
#define NIS_QT_VUE_IMAGE_H

#include <QWidget>
#include <QImage>
#include <vector>

#include "core/Types.h"

class VueImage : public QWidget {
  Q_OBJECT
 public:
  explicit VueImage(QWidget* parent = nullptr);

  void definirImage(const QImage& image, const QString& legende = QString());
  void definirMinuties(const std::vector<nis::Sommet>& minuties);
  void effacer();

  QSize sizeHint() const override;

 protected:
  void paintEvent(QPaintEvent* evenement) override;

 private:
  QImage image_;
  QString legende_;
  std::vector<nis::Sommet> minuties_;
};

#endif  // NIS_QT_VUE_IMAGE_H
