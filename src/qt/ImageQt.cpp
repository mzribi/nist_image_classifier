#include "ImageQt.h"

#include <QDir>
#include <QImage>

namespace pontqt {

QImage versQImage(const nis::ImageNiveauxGris& image) {
  QImage sortie(static_cast<int>(image.largeur()), static_cast<int>(image.hauteur()),
                QImage::Format_RGB32);
  if (image.vide()) return sortie;
  for (std::size_t y = 0; y < image.hauteur(); ++y) {
    for (std::size_t x = 0; x < image.largeur(); ++x) {
      const int g = static_cast<int>(image.au(x, y));
      sortie.setPixel(static_cast<int>(x), static_cast<int>(y), qRgb(g, g, g));
    }
  }
  return sortie;
}

bool depuisQImage(const QImage& entree, nis::ImageNiveauxGris* sortie, std::string* erreur) {
  if (sortie == nullptr) return false;
  if (entree.isNull()) {
    if (erreur) *erreur = "image vide";
    return false;
  }
  const QImage convertie = entree.convertToFormat(QImage::Format_RGB32);
  nis::ImageNiveauxGris image(static_cast<std::size_t>(convertie.width()),
                              static_cast<std::size_t>(convertie.height()));
  for (int y = 0; y < convertie.height(); ++y) {
    for (int x = 0; x < convertie.width(); ++x) {
      image(static_cast<std::size_t>(x), static_cast<std::size_t>(y)) =
          static_cast<std::uint8_t>(qGray(convertie.pixel(x, y)));
    }
  }
  *sortie = std::move(image);
  return true;
}

nis::ChargeurImage chargeurFichier() {
  return [](const std::string& chemin, nis::ImageNiveauxGris* sortie,
            std::string* erreur) {
    QImage chargee;
    if (!chargee.load(QString::fromStdString(chemin))) {
      if (erreur) *erreur = "lecture impossible (format non pris en charge ou fichier absent)";
      return false;
    }
    return depuisQImage(chargee, sortie, erreur);
  };
}

QString filtreOuverture() {
  return QString::fromUtf8("Images (*.png *.bmp *.jpg *.jpeg *.ppm *.pgm *.tif *.tiff);;") +
         QString::fromUtf8("Tous les fichiers (*.*)");
}

QStringList motifsFichiers(const std::vector<std::string>& extensions) {
  QStringList motifs;
  for (const std::string& ext : extensions) {
    const QString e = QString::fromStdString(ext).toLower();
    motifs << ("*." + e) << ("*." + e.toUpper());
  }
  return motifs;
}

QStringList listerImages(const QString& dossier, const std::vector<std::string>& extensions) {
  const QDir repertoire(dossier);
  if (!repertoire.exists()) return {};
  QStringList relatifs =
      repertoire.entryList(motifsFichiers(extensions), QDir::Files | QDir::Readable, QDir::Name);
  QStringList chemins;
  for (const QString& relatif : relatifs) chemins << repertoire.absoluteFilePath(relatif);
  return chemins;
}

}  // namespace pontqt
