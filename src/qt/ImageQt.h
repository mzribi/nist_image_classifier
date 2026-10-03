// Pont entre le noyau de calcul et Qt : conversion QImage <-> ImageNiveauxGris
// et chargeur de fichiers injecte dans ConstructeurBase.
//
// Remplace le chargement OpenCV (`cvLoadImage`) et l'affichage MFC (`CImage`,
// `StretchDraw`) : un seul endroit connait QImage, le noyau reste pur C++17.
//
// Compatibilite Qt 5.12 / Qt 6 : on n'utilise que QImage::Format_RGB32 et
// qGray (QImage::Format_Grayscale8 n'existe que depuis Qt 5.13).
#ifndef NIS_QT_IMAGE_QT_H
#define NIS_QT_IMAGE_QT_H

#include <QString>
#include <QStringList>
#include <string>
#include <vector>

#include "core/ConstructeurBase.h"

class QImage;

namespace pontqt {

/// Image du noyau vers QImage affichable (gris 8 bits dans un RGB32).
QImage versQImage(const nis::ImageNiveauxGris& image);

/// QImage (couleur ou gris, toute profondeur) vers niveaux de gris du noyau.
bool depuisQImage(const QImage& entree, nis::ImageNiveauxGris* sortie,
                  std::string* erreur = nullptr);

/// Chargeur de fichiers pour ConstructeurBase, base sur QImage (PNG, BMP, JPG,
/// PPM, TIFF selon les plugins Qt presents).
nis::ChargeurImage chargeurFichier();

/// Filtre "Images (*.png *.jpg ...);;Tous (*.*)" pour les dialogues Fichier.
QString filtreOuverture();

/// Motifs "*.ext" (minuscules et majuscules) pour QDir::entryList.
QStringList motifsFichiers(const std::vector<std::string>& extensions);

/// Chemins complets des images d'un dossier, tries par nom.
QStringList listerImages(const QString& dossier, const std::vector<std::string>& extensions);

}  // namespace pontqt

#endif  // NIS_QT_IMAGE_QT_H
