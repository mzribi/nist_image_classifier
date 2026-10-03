// Panneau « Classification d'une base de donnees d'images » : port de
// IDD_CLASSER_BD (CbdimgDlg) + IDD_DIALOG2 (Ccommencer).
//
// Memes champs, memes libelles : types JPG/PNG/BMP/PPM/TIF (2006 :
// PPM/JPG/TIF/BMP), repertoire d'images + « Parcourir... », nombre de classes
// (2..6 comme en 2006, modifiable), « Continuer » puis dossier de sortie.
// La fausse barre Sleep(400) de 2006 est remplacee par la progression REELLE
// du noyau et un vrai bouton Annuler.
#ifndef NIS_QT_PANEL_BASE_H
#define NIS_QT_PANEL_BASE_H

#include <QWidget>
#include <QStringList>
#include <memory>
#include <vector>

#include "core/ConstructeurBase.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QThread;
class TravailleurBase;

class PanelBase : public QWidget {
  Q_OBJECT
 public:
  explicit PanelBase(QWidget* parent = nullptr);
  ~PanelBase() override;

  std::vector<nis::Graphe> centres() const;

 signals:
  void message(const QString& texte);
  void centresPrets(const std::vector<nis::Graphe>& centres);

 private slots:
  void parcourir();
  void lancer();
  void annuler();
  void ouvrirSortie();
  void surProgression(int pourcentage, const QString& libelle);
  void surTermine(bool annule);

 private:
  std::vector<std::string> extensionsChoisies() const;
  void mettreAJourBoutons(bool enCours);
  void remplirTable(const nis::ResultatBase& resultat);

  QLineEdit* dossier_ = nullptr;
  QComboBox* nbClasses_ = nullptr;
  QCheckBox* extJpg_ = nullptr;
  QCheckBox* extPng_ = nullptr;
  QCheckBox* extBmp_ = nullptr;
  QCheckBox* extPpm_ = nullptr;
  QCheckBox* extTif_ = nullptr;
  QPushButton* btnContinuer_ = nullptr;
  QPushButton* btnAnnuler_ = nullptr;
  QPushButton* btnSortie_ = nullptr;
  QProgressBar* progression_ = nullptr;
  QLabel* statut_ = nullptr;
  QTableWidget* table_ = nullptr;

  QThread* fil_ = nullptr;
  TravailleurBase* travailleur_ = nullptr;
  QString dossierSortie_;
  std::shared_ptr<nis::ResultatBase> dernierResultat_;
};

#endif  // NIS_QT_PANEL_BASE_H
