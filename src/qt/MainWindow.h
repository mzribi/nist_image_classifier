// Fenetre principale : onglets « Image » et « Base de donnees » + menus.
//
// Les menus reprennent l'application MFC (Fichier, Traitement, Aide) et
// ouvrent les dialogues modaux historiques ; les onglets offrent les memes
// panneaux en persistant.  Les centres calcules par l'onglet Base sont
// propages a l'onglet Image pour la classification d'une image isolee.
#ifndef NIS_QT_MAIN_WINDOW_H
#define NIS_QT_MAIN_WINDOW_H

#include <QMainWindow>
#include <vector>

#include "core/ConstructeurBase.h"

class PanelBase;
class PanelImage;

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private slots:
  void ouvrirDialogueImage();
  void ouvrirDialogueBase();
  void recevoirCentres(const std::vector<nis::Graphe>& centres);
  void afficherMessage(const QString& texte);

 private:
  PanelImage* ongletImage_ = nullptr;
  PanelBase* ongletBase_ = nullptr;
  std::vector<nis::Graphe> centres_;
};

#endif  // NIS_QT_MAIN_WINDOW_H
