#include "PanelImage.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "ImageQt.h"
#include "VueImage.h"

PanelImage::PanelImage(QWidget* parent) : QWidget(parent) {
  auto* disposition = new QHBoxLayout(this);

  auto* colonne = new QWidget(this);
  auto* boutons = new QVBoxLayout(colonne);
  auto* btnOuvrir = new QPushButton(QString::fromUtf8("Ouvrir Image"), colonne);
  btnBinariser_ = new QPushButton(QString::fromUtf8("Binarisation"), colonne);
  btnFond_ = new QPushButton(QString::fromUtf8("Elimination de fond"), colonne);
  btnMince_ = new QPushButton(QString::fromUtf8("Miniaturisation"), colonne);
  btnMinuties_ = new QPushButton(QString::fromUtf8("Extraction des minuties"), colonne);
  btnClassifier_ = new QPushButton(QString::fromUtf8("Classification"), colonne);
  btnEnregistrer_ = new QPushButton(QString::fromUtf8("Enregistrer l'image"), colonne);
  seuil_ = new QSpinBox(colonne);
  seuil_->setRange(1, 255);
  seuil_->setValue(128);
  seuil_->setPrefix(QString::fromUtf8("Seuil "));
  for (QPushButton* b : {btnOuvrir, btnBinariser_, btnFond_, btnMince_, btnMinuties_,
                          btnClassifier_, btnEnregistrer_}) {
    b->setMinimumWidth(170);
    boutons->addWidget(b);
  }
  boutons->addWidget(seuil_);
  boutons->addStretch(1);
  disposition->addWidget(colonne);

  auto* droite = new QWidget(this);
  auto* vueDisposition = new QVBoxLayout(droite);
  vue_ = new VueImage(droite);
  info_ = new QLabel(droite);
  info_->setWordWrap(true);
  vueDisposition->addWidget(vue_, 1);
  vueDisposition->addWidget(info_);
  disposition->addWidget(droite, 1);

  connect(btnOuvrir, &QPushButton::clicked, this, &PanelImage::ouvrirImage);
  connect(btnBinariser_, &QPushButton::clicked, this, &PanelImage::binariser);
  connect(btnFond_, &QPushButton::clicked, this, &PanelImage::eliminerFond);
  connect(btnMince_, &QPushButton::clicked, this, &PanelImage::miniaturiser);
  connect(btnMinuties_, &QPushButton::clicked, this, &PanelImage::extraireMinuties);
  connect(btnClassifier_, &QPushButton::clicked, this, &PanelImage::classifier);
  connect(btnEnregistrer_, &QPushButton::clicked, this, &PanelImage::enregistrerImage);
  connect(seuil_, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          [this](int v) { options_.seuil = v; });
  mettreAJourBoutons();
}

void PanelImage::definirCentres(const std::vector<nis::Graphe>& centres) {
  centres_ = centres;
  mettreAJourBoutons();
}

void PanelImage::ouvrirImage() {
  const QString chemin = QFileDialog::getOpenFileName(this, QString::fromUtf8("Ouvrir"),
                                                      cheminCourant_, pontqt::filtreOuverture());
  if (chemin.isEmpty()) return;
  QImage chargee;
  if (!chargee.load(chemin)) {
    QMessageBox::warning(this, QString::fromUtf8("Ouvrir"),
                         QString::fromUtf8("Lecture impossible de l'image."));
    return;
  }
  std::string erreur;
  if (!pontqt::depuisQImage(chargee, &courante_, &erreur)) {
    QMessageBox::warning(this, QString::fromUtf8("Ouvrir"),
                         QString::fromStdString(erreur));
    return;
  }
  originale_ = chargee;
  cheminCourant_ = chemin;
  aImage_ = true;
  minuties_.clear();
  graphe_ = nis::Graphe();
  vue_->definirImage(originale_, QFileInfo(chemin).fileName());
  vue_->definirMinuties({});
  afficherEtape(QString::fromUtf8("Image ouverte : ") + QFileInfo(chemin).fileName());
  mettreAJourBoutons();
}

void PanelImage::binariser() {
  if (!aImage_) return;
  courante_ = nis::pipeline::binariser(courante_, options_.seuil);
  minuties_.clear();
  afficherEtape(QString::fromUtf8("Binarisation (seuil ") + QString::number(options_.seuil) +
                QString::fromUtf8(")"));
}

void PanelImage::eliminerFond() {
  if (!aImage_) return;
  courante_ = nis::pipeline::eliminerFond(courante_);
  minuties_.clear();
  afficherEtape(QString::fromUtf8("Elimination de fond"));
}

void PanelImage::miniaturiser() {
  if (!aImage_) return;
  const nis::EtapePipeline etape = nis::pipeline::amincir(&courante_, options_);
  minuties_.clear();
  afficherEtape(QString::fromUtf8("Miniaturisation : ") + QString::number(etape.passes) +
                QString::fromUtf8(" passes") + (etape.converge ? QString() : QString::fromUtf8(" (non converge)")));
}

void PanelImage::extraireMinuties() {
  if (!aImage_) return;
  minuties_ = nis::pipeline::extraireMinuties(courante_, options_);
  recalculerGraphe();
  vue_->definirMinuties(minuties_);
  int term = 0, bif = 0;
  for (const nis::Sommet& s : minuties_) {
    if (s.type == nis::TypeMinutie::Terminaison) ++term;
    else ++bif;
  }
  afficherEtape(QString::fromUtf8("Extraction des minuties : ") + QString::number(minuties_.size()) +
                QString::fromUtf8(" (") + QString::number(term) +
                QString::fromUtf8(" terminaisons, ") + QString::number(bif) +
                QString::fromUtf8(" bifurcations)"));
  mettreAJourBoutons();
}

void PanelImage::classifier() {
  if (centres_.empty()) {
    QMessageBox::information(this, QString::fromUtf8("Classification"),
                             QString::fromUtf8("Aucun centre disponible : lancez d'abord la classification d'une base."));
    return;
  }
  if (graphe_.nbSommets() == 0) recalculerGraphe();
  if (graphe_.nbSommets() == 0) {
    QMessageBox::information(this, QString::fromUtf8("Classification"),
                             QString::fromUtf8("Extrayez d'abord les minuties de l'image."));
    return;
  }
  double erreur = 0.0;
  nis::OptionsClassification oc;
  const int rang = nis::Classeur::classeLaPlusProche(graphe_, centres_, oc.median.appariement, &erreur);
  if (rang < 0) {
    QMessageBox::warning(this, QString::fromUtf8("Classification"),
                         QString::fromUtf8("Classement impossible."));
    return;
  }
  const QString texte = QString::fromUtf8("Image classe en « classe ") + QString::number(rang + 1) +
                        QString::fromUtf8(" » (erreur ") + QString::number(erreur, 'f', 2) +
                        QString::fromUtf8(")");
  afficherEtape(texte);
  emit message(texte);
}

void PanelImage::enregistrerImage() {
  if (!aImage_) return;
  const QString chemin = QFileDialog::getSaveFileName(
      this, QString::fromUtf8("Enregistrer l'image"), cheminCourant_,
      QString::fromUtf8("PNG (*.png);;BMP (*.bmp);;JPEG (*.jpg)"));
  if (chemin.isEmpty()) return;
  if (!pontqt::versQImage(courante_).save(chemin)) {
    QMessageBox::warning(this, QString::fromUtf8("Enregistrer l'image"),
                         QString::fromUtf8("Ecriture impossible."));
    return;
  }
  emit message(QString::fromUtf8("Image enregistree : ") + chemin);
}

void PanelImage::afficherEtape(const QString& titre) {
  vue_->definirImage(pontqt::versQImage(courante_), titre);
  vue_->definirMinuties(minuties_);
  info_->setText(titre);
  emit message(titre);
}

void PanelImage::recalculerGraphe() {
  graphe_ = nis::pipeline::construireGraphe(
      minuties_, cheminCourant_.toStdString());
}

void PanelImage::mettreAJourBoutons() {
  const bool image = aImage_;
  btnBinariser_->setEnabled(image);
  btnFond_->setEnabled(image);
  btnMince_->setEnabled(image);
  btnMinuties_->setEnabled(image);
  btnEnregistrer_->setEnabled(image);
  btnClassifier_->setEnabled(image && !centres_.empty());
}
