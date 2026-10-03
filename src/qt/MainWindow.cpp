#include "MainWindow.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>

#include "Dialogs.h"
#include "PanelBase.h"
#include "PanelImage.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QString::fromUtf8("nis - Classification d'images par graphes de minuties"));

  auto* onglets = new QTabWidget(this);
  ongletImage_ = new PanelImage(onglets);
  ongletBase_ = new PanelBase(onglets);
  onglets->addTab(ongletImage_, QString::fromUtf8("Image"));
  onglets->addTab(ongletBase_, QString::fromUtf8("Base de données"));
  setCentralWidget(onglets);

  auto* menuFichier = menuBar()->addMenu(QString::fromUtf8("Fichier"));
  auto* actionQuitter = menuFichier->addAction(QString::fromUtf8("Quitter"));
  actionQuitter->setShortcut(QKeySequence::Quit);
  connect(actionQuitter, &QAction::triggered, this, &QWidget::close);

  auto* menuTraitement = menuBar()->addMenu(QString::fromUtf8("Traitement"));
  auto* actionImage = menuTraitement->addAction(QString::fromUtf8("Traitement d'une image..."));
  auto* actionBase = menuTraitement->addAction(
      QString::fromUtf8("Classification d'une base de données d'images..."));
  connect(actionImage, &QAction::triggered, this, &MainWindow::ouvrirDialogueImage);
  connect(actionBase, &QAction::triggered, this, &MainWindow::ouvrirDialogueBase);

  auto* menuAide = menuBar()->addMenu(QString::fromUtf8("Aide"));
  auto* actionPropos = menuAide->addAction(QString::fromUtf8("A propos de notre projet"));
  connect(actionPropos, &QAction::triggered, [this] { afficherAPropos(this); });

  connect(ongletImage_, &PanelImage::message, this, &MainWindow::afficherMessage);
  connect(ongletBase_, &PanelBase::message, this, &MainWindow::afficherMessage);
  connect(ongletBase_, &PanelBase::centresPrets, this, &MainWindow::recevoirCentres);
  statusBar()->showMessage(QString::fromUtf8("Pret."));
  resize(1024, 720);
}

void MainWindow::ouvrirDialogueImage() {
  DialogueTraitementImage dialogue(centres_, this);
  dialogue.exec();
}

void MainWindow::ouvrirDialogueBase() {
  DialogueClassification dialogue(this);
  connect(&dialogue, &DialogueClassification::centresPrets, this, &MainWindow::recevoirCentres);
  dialogue.exec();
}

void MainWindow::recevoirCentres(const std::vector<nis::Graphe>& centres) {
  centres_ = centres;
  ongletImage_->definirCentres(centres_);
  statusBar()->showMessage(QString::fromUtf8("Centres memorises : ") +
                           QString::number(static_cast<int>(centres_.size())) +
                           QString::fromUtf8(" classe(s)."));
}

void MainWindow::afficherMessage(const QString& texte) { statusBar()->showMessage(texte, 8000); }
