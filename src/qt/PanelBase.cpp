#include "PanelBase.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

#include "ImageQt.h"
#include "TravailleurBase.h"

PanelBase::PanelBase(QWidget* parent) : QWidget(parent) {
  auto* disposition = new QVBoxLayout(this);

  auto* groupe = new QWidget(this);
  auto* grille = new QGridLayout(groupe);
  int ligne = 0;
  grille->addWidget(new QLabel(QString::fromUtf8("Donnez le type des images a traiter"), groupe), ligne, 0, 1, 4);
  ++ligne;
  extJpg_ = new QCheckBox(QString::fromUtf8("JPG"), groupe);
  extPng_ = new QCheckBox(QString::fromUtf8("PNG"), groupe);
  extBmp_ = new QCheckBox(QString::fromUtf8("BMP"), groupe);
  extPpm_ = new QCheckBox(QString::fromUtf8("PPM"), groupe);
  extTif_ = new QCheckBox(QString::fromUtf8("TIF"), groupe);
  extJpg_->setChecked(true);
  extPng_->setChecked(true);
  grille->addWidget(extJpg_, ligne, 0);
  grille->addWidget(extPng_, ligne, 1);
  grille->addWidget(extBmp_, ligne, 2);
  grille->addWidget(extPpm_, ligne, 3);
  grille->addWidget(extTif_, ligne, 4);
  ++ligne;
  grille->addWidget(new QLabel(QString::fromUtf8("Donnez le repertoire d'image"), groupe), ligne, 0, 1, 2);
  dossier_ = new QLineEdit(groupe);
  dossier_->setReadOnly(true);
  grille->addWidget(dossier_, ligne, 2, 1, 2);
  auto* btnParcourir = new QPushButton(QString::fromUtf8("Parcourir..."), groupe);
  grille->addWidget(btnParcourir, ligne, 4);
  ++ligne;
  grille->addWidget(new QLabel(QString::fromUtf8("Donnez le nombre de classes"), groupe), ligne, 0, 1, 2);
  nbClasses_ = new QComboBox(groupe);
  for (int k = 2; k <= 6; ++k) nbClasses_->addItem(QString::number(k));
  grille->addWidget(nbClasses_, ligne, 2);
  ++ligne;
  btnContinuer_ = new QPushButton(QString::fromUtf8("Continuer"), groupe);
  btnAnnuler_ = new QPushButton(QString::fromUtf8("Annuler"), groupe);
  btnSortie_ = new QPushButton(QString::fromUtf8("Ouvrir le dossier de sortie"), groupe);
  grille->addWidget(btnContinuer_, ligne, 0, 1, 2);
  grille->addWidget(btnAnnuler_, ligne, 2, 1, 2);
  grille->addWidget(btnSortie_, ligne, 4);
  disposition->addWidget(groupe);

  progression_ = new QProgressBar(this);
  progression_->setRange(0, 100);
  statut_ = new QLabel(this);
  statut_->setWordWrap(true);
  disposition->addWidget(progression_);
  disposition->addWidget(statut_);

  table_ = new QTableWidget(this);
  table_->setColumnCount(4);
  table_->setHorizontalHeaderLabels({QString::fromUtf8("Image"), QString::fromUtf8("Classe"),
                                     QString::fromUtf8("Erreur"), QString::fromUtf8("Sommets")});
  table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table_->setEditTriggers(QTableWidget::NoEditTriggers);
  disposition->addWidget(table_, 1);

  fil_ = new QThread(this);
  travailleur_ = new TravailleurBase();
  travailleur_->moveToThread(fil_);
  connect(fil_, &QThread::finished, travailleur_, &QObject::deleteLater);
  connect(this, &PanelBase::destroyed, this, [this] {
    if (fil_->isRunning()) {
      travailleur_->demanderArret();
      fil_->quit();
      fil_->wait(3000);
    }
  });
  connect(travailleur_, &TravailleurBase::progression, this, &PanelBase::surProgression);
  connect(travailleur_, &TravailleurBase::termine, this, &PanelBase::surTermine);
  fil_->start();

  connect(btnParcourir, &QPushButton::clicked, this, &PanelBase::parcourir);
  connect(btnContinuer_, &QPushButton::clicked, this, &PanelBase::lancer);
  connect(btnAnnuler_, &QPushButton::clicked, this, &PanelBase::annuler);
  connect(btnSortie_, &QPushButton::clicked, this, &PanelBase::ouvrirSortie);
  connect(dossier_, &QLineEdit::textChanged, [this] { mettreAJourBoutons(false); });
  mettreAJourBoutons(false);
}

PanelBase::~PanelBase() {
  if (fil_ != nullptr && fil_->isRunning()) {
    travailleur_->demanderArret();
    fil_->quit();
    fil_->wait(5000);
  }
}

std::vector<nis::Graphe> PanelBase::centres() const {
  if (!dernierResultat_) return {};
  return dernierResultat_->centres();
}

std::vector<std::string> PanelBase::extensionsChoisies() const {
  std::vector<std::string> ext;
  if (extJpg_->isChecked()) { ext.push_back("jpg"); ext.push_back("jpeg"); }
  if (extPng_->isChecked()) ext.push_back("png");
  if (extBmp_->isChecked()) ext.push_back("bmp");
  if (extPpm_->isChecked()) { ext.push_back("ppm"); ext.push_back("pgm"); }
  if (extTif_->isChecked()) { ext.push_back("tif"); ext.push_back("tiff"); }
  return ext;
}

void PanelBase::parcourir() {
  const QString dossier = QFileDialog::getExistingDirectory(this, QString::fromUtf8("Ouvrir"), dossier_->text());
  if (dossier.isEmpty()) return;
  dossier_->setText(dossier);
  mettreAJourBoutons(false);
}

void PanelBase::lancer() {
  const QString dossier = dossier_->text();
  if (dossier.isEmpty()) {
    QMessageBox::information(this, QString::fromUtf8("Classification d'une base de donnees d'images"),
                             QString::fromUtf8("Choisissez d'abord un repertoire d'images (Parcourir...)."));
    return;
  }
  const std::vector<std::string> ext = extensionsChoisies();
  if (ext.empty()) {
    QMessageBox::information(this, QString::fromUtf8("Classification d'une base de donnees d'images"),
                             QString::fromUtf8("Cochez au moins un type d'images."));
    return;
  }
  const QStringList fichiers = pontqt::listerImages(dossier, ext);
  if (fichiers.isEmpty()) {
    QMessageBox::information(this, QString::fromUtf8("Classification d'une base de donnees d'images"),
                             QString::fromUtf8("Aucune image de ce type dans ce repertoire."));
    return;
  }
  int nb = nbClasses_->currentText().toInt();
  if (nb < 2) nb = 2;
  if (nb > static_cast<int>(fichiers.size())) {
    QMessageBox::information(
        this, QString::fromUtf8("Classification d'une base de donnees d'images"),
        QString::fromUtf8("Moins d'images que de classes : le nombre de classes est ramene a ") +
            QString::number(fichiers.size()) + QString::fromUtf8("."));
    nb = static_cast<int>(fichiers.size());
  }
  dossierSortie_ = dossier + QString::fromUtf8("/nis_sortie");
  nis::OptionsBase options;
  options.classification.nb_classes = static_cast<std::size_t>(nb);
  options.copier_images = true;
  dernierResultat_.reset();
  table_->setRowCount(0);
  mettreAJourBoutons(true);
  statut_->setText(QString::fromUtf8("Classification en cours..."));
  // Configuration thread-safe : on passe par une invocation dans le fil.
  QMetaObject::invokeMethod(travailleur_, [this, options, fichiers] {
    travailleur_->configurer(options, fichiers, dossierSortie_);
    QMetaObject::invokeMethod(travailleur_, "executer", Qt::QueuedConnection);
  }, Qt::QueuedConnection);
}

void PanelBase::annuler() {
  if (travailleur_ != nullptr) travailleur_->demanderArret();
  statut_->setText(QString::fromUtf8("Annulation demandee..."));
}

void PanelBase::ouvrirSortie() {
  if (dossierSortie_.isEmpty()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(dossierSortie_));
}

void PanelBase::surProgression(int pourcentage, const QString& libelle) {
  progression_->setValue(pourcentage);
  statut_->setText(libelle);
}

void PanelBase::surTermine(bool annule) {
  mettreAJourBoutons(false);
  dernierResultat_ = travailleur_->resultat();
  if (!dernierResultat_) {
    statut_->setText(QString::fromUtf8("Aucun resultat."));
    return;
  }
  remplirTable(*dernierResultat_);
  if (annule || dernierResultat_->annule) {
    statut_->setText(QString::fromUtf8("Classification annulee."));
    emit message(QString::fromUtf8("Classification annulee."));
    return;
  }
  QString resume = QString::fromUtf8("La classification de la base est terminee : ") +
                   QString::number(dernierResultat_->images.size()) +
                   QString::fromUtf8(" images, ") +
                   QString::number(dernierResultat_->classes.size()) +
                   QString::fromUtf8(" classes dans ") + dossierSortie_;
  if (!dernierResultat_->avertissements.empty()) {
    resume += QString::fromUtf8(" (") +
              QString::number(dernierResultat_->avertissements.size()) +
              QString::fromUtf8(" avertissement(s))");
  }
  statut_->setText(resume);
  emit message(resume);
  emit centresPrets(dernierResultat_->centres());
  QMessageBox::information(this,
                           QString::fromUtf8("Classification d'une base de donnees d'images"),
                           QString::fromUtf8("La classification de la base est terminee."));
}

void PanelBase::mettreAJourBoutons(bool enCours) {
  btnContinuer_->setEnabled(!enCours && !dossier_->text().isEmpty());
  btnAnnuler_->setEnabled(enCours);
  btnSortie_->setEnabled(!enCours && !dossierSortie_.isEmpty());
}

void PanelBase::remplirTable(const nis::ResultatBase& resultat) {
  table_->setRowCount(static_cast<int>(resultat.images.size()));
  for (int i = 0; i < static_cast<int>(resultat.images.size()); ++i) {
    const nis::ResultatImage& e = resultat.images[static_cast<std::size_t>(i)];
    auto* nom = new QTableWidgetItem(QFileInfo(QString::fromStdString(e.fichier)).fileName());
    nom->setToolTip(QString::fromStdString(e.fichier));
    table_->setItem(i, 0, nom);
    table_->setItem(i, 1, new QTableWidgetItem(e.classe < 0 ? QString::fromUtf8("-")
                                                            : QString::number(e.classe + 1)));
    table_->setItem(i, 2, new QTableWidgetItem(e.echec.empty() ? QString::number(e.erreur, 'f', 2)
                                                               : QString::fromStdString(e.echec)));
    table_->setItem(i, 3, new QTableWidgetItem(QString::number(e.graphe.nbSommets())));
  }
}
