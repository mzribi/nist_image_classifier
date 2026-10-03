// Image 8 bits un canal, independante de toute bibliotheque d'affichage.
//
// Le noyau de calcul ne manipule que ce type : c'est ce qui permet de tester
// toute la chaine (binarisation -> amincissement -> minuties) sans ecran ni Qt.
// La conversion avec `QImage` est faite dans la couche interface
// (`src/qt/PontQt.h`).
//
// Convention reprise de 2006 : le fond est blanc (255), l'encre (le trait) est
// noire (0).
#ifndef NIS_CORE_IMAGE_NIVEAUX_GRIS_H
#define NIS_CORE_IMAGE_NIVEAUX_GRIS_H

#include <cstdint>
#include <vector>

namespace nis {

class ImageNiveauxGris {
 public:
  static constexpr std::uint8_t Fond = 255;
  static constexpr std::uint8_t Encre = 0;

  ImageNiveauxGris() = default;
  ImageNiveauxGris(std::size_t largeur, std::size_t hauteur, std::uint8_t valeur = Fond);

  /// Copie des pixels dans une image ligne par ligne, sans remplitage.
  static ImageNiveauxGris depuisLignes(const std::vector<std::uint8_t>& pixels,
                                       std::size_t largeur, std::size_t hauteur);

  std::size_t largeur() const { return largeur_; }
  std::size_t hauteur() const { return hauteur_; }
  std::size_t taille() const { return pixels_.size(); }
  bool vide() const { return largeur_ == 0 || hauteur_ == 0; }

  const std::vector<std::uint8_t>& pixels() const { return pixels_; }
  std::vector<std::uint8_t>& pixels() { return pixels_; }
  const std::uint8_t* donnees() const { return pixels_.data(); }
  std::uint8_t* donnees() { return pixels_.data(); }

  /// Acces (x, y) sans verification : a n'utiliser que dans des boucles bornees.
  std::uint8_t& operator()(std::size_t x, std::size_t y) { return pixels_[y * largeur_ + x]; }
  std::uint8_t operator()(std::size_t x, std::size_t y) const { return pixels_[y * largeur_ + x]; }

  /// Acces sur avec verification.  Hors image : `Fond`, ce qui evite les lectures
  /// hors limites qui hantaient l'ancien code (`image[i+1][j+1]` sur le bord).
  std::uint8_t au(std::size_t x, std::size_t y) const {
    if (x >= largeur_ || y >= hauteur_) return Fond;
    return pixels_[y * largeur_ + x];
  }
  void set(std::size_t x, std::size_t y, std::uint8_t valeur) {
    if (x < largeur_ && y < hauteur_) pixels_[y * largeur_ + x] = valeur;
  }

  /// Vrai si le niveau de gris designe le trait (et non le fond).
  static bool estEncre(std::uint8_t valeur) { return valeur < 128; }
  bool estEncre(std::size_t x, std::size_t y) const { return estEncre(au(x, y)); }

  void remplir(std::uint8_t valeur);
  std::size_t nbEncres() const;

 private:
  std::size_t largeur_{0};
  std::size_t hauteur_{0};
  std::vector<std::uint8_t> pixels_;
};

}  // namespace nis

#endif  // NIS_CORE_IMAGE_NIVEAUX_GRIS_H
