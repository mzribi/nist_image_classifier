// Dialogues modaux : ports directs des boites MFC, construits autour des
// MEMES panneaux que les onglets (aucun code duplique).
//
//   DialogueTraitementImage  <-> IDD_CLASSER_IMG  (CimgDlg)
//   DialogueClassification   <-> IDD_CLASSER_BD   (CbdimgDlg) + progression
//   DialogueAPropos          <-> IDD_ABOUTBOX     (« Projet de Deux Modules,
//                                                   ENSI Copyright (C) 2006 »)
#ifndef NIS_QT_DIALOGS_H
#define NIS_QT_DIALOGS_H

#include <QDialog>
#include <vector>

#include "core/ConstructeurBase.h"

class PanelBase;
class PanelImage;
class QVBoxLayout;

class DialogueTraitementImage : public QDialog {
  Q_OBJECT
 public:
  explicit DialogueTraitementImage(const std::vector<nis::Graphe>& centres,
                                   QWidget* parent = nullptr);

 private:
  PanelImage* panneau_ = nullptr;
};

class DialogueClassification : public QDialog {
  Q_OBJECT
 public:
  explicit DialogueClassification(QWidget* parent = nullptr);

 signals:
  void centresPrets(const std::vector<nis::Graphe>& centres);

 private:
  PanelBase* panneau_ = nullptr;
};

void afficherAPropos(QWidget* parent);

#endif  // NIS_QT_DIALOGS_H
