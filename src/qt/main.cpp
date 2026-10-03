// Point d'entree de l'application graphique nis.
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char** argv) {
  QApplication application(argc, argv);
  application.setApplicationName(QString::fromUtf8("nis"));
  application.setOrganizationName(QString::fromUtf8("ENSI-2006"));
  MainWindow fenetre;
  fenetre.show();
  return application.exec();
}
