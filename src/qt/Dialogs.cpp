#include "Dialogs.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "PanelBase.h"
#include "PanelImage.h"

DialogueTraitementImage::DialogueTraitementImage(const std::vector<nis::Graphe>& centres,
                                                 QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(QString::fromUtf8("Traitement d'une image"));
  auto* disposition = new QVBoxLayout(this);
  panneau_ = new PanelImage(this);
  panneau_->definirCentres(centres);
  disposition->addWidget(panneau_, 1);
  auto* boutons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  boutons->button(QDialogButtonBox::Close)->setText(QString::fromUtf8("Quitter"));
  connect(boutons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  disposition->addWidget(boutons);
  resize(860, 620);
}

DialogueClassification::DialogueClassification(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QString::fromUtf8("Classification d'une base de données d'images"));
  auto* disposition = new QVBoxLayout(this);
  panneau_ = new PanelBase(this);
  disposition->addWidget(panneau_, 1);
  auto* boutons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  boutons->button(QDialogButtonBox::Close)->setText(QString::fromUtf8("Retour"));
  connect(boutons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  disposition->addWidget(boutons);
  connect(panneau_, &PanelBase::centresPrets, this, &DialogueClassification::centresPrets);
  resize(900, 640);
}

void afficherAPropos(QWidget* parent) {
  QMessageBox::about(parent, QString::fromUtf8("A propos de notre projet"),
                     QString::fromUtf8("Projet de Deux Modules\nENSI Copyright (C) 2006\n") +
                         QString::fromUtf8("Portage Qt 5/6 + CMake : classification d'images ") +
                         QString::fromUtf8("par graphes de minuties."));
}
