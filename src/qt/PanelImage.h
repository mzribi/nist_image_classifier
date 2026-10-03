// Panneau « Traitement d'une image » : port de IDD_CLASSER_IMG (CimgDlg).
//
// Memes etapes, memes libelles : Ouvrir Image, Binarisation, Elimination de
// fond, Miniaturisation (amincissement), Extraction des minuties,
// Classification, Enregistrer l'image.  Chaque etape affiche son resultat et
// le compteur de minuties tient lieu de la zone de texte en lecture seule.
//
// Le widget est autonome : la fenetre principale l'affiche dans un onglet, les
// menus l'encapsulent dans un dialogue modal (Dialogs.h).  Les centres de
// classification sont recus de l'exterieur (onglet Base ou dialogue).
#ifndef NIS_QT_PANEL_IMAGE_H
#define NIS_QT_PANEL_IMAGE_H

#include <QWidget>
#include <QImage>
#include <vector>

#include "core/ConstructeurBase.h"
#include "core/PipelineImage.h"

class QLabel;
class QPushButton;
class QSpinBox;
class VueImage;

class PanelImage : public QWidget {
  Q_OBJECT
 public:
  explicit PanelImage(QWidget* parent = nullptr);

  void definirCentres(const std::vector<nis::Graphe>& centres);

 signals:
  void message(const QString& texte);

 private slots:
  void ouvrirImage();
  void binariser();
  void eliminerFond();
  void miniaturiser();
  void extraireMinuties();
  void classifier();
  void enregistrerImage();

 private:
  void afficherEtape(const QString& titre);
  void recalculerGraphe();
  void mettreAJourBoutons();

  VueImage* vue_ = nullptr;
  QLabel* info_ = nullptr;
  QSpinBox* seuil_ = nullptr;
  QPushButton* btnBinariser_ = nullptr;
  QPushButton* btnFond_ = nullptr;
  QPushButton* btnMince_ = nullptr;
  QPushButton* btnMinuties_ = nullptr;
  QPushButton* btnClassifier_ = nullptr;
  QPushButton* btnEnregistrer_ = nullptr;

  QImage originale_;
  QString cheminCourant_;
  nis::ImageNiveauxGris courante_;
  bool aImage_ = false;
  std::vector<nis::Sommet> minuties_;
  nis::Graphe graphe_;
  nis::OptionsPipeline options_;
  std::vector<nis::Graphe> centres_;
};

#endif  // NIS_QT_PANEL_IMAGE_H
