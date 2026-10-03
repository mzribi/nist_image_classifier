#include "core/ImageNiveauxGris.h"

#include <algorithm>

namespace nis {

ImageNiveauxGris::ImageNiveauxGris(std::size_t largeur, std::size_t hauteur, std::uint8_t valeur)
    : largeur_(largeur), hauteur_(hauteur), pixels_(largeur * hauteur, valeur) {}

ImageNiveauxGris ImageNiveauxGris::depuisLignes(const std::vector<std::uint8_t>& pixels,
                                                std::size_t largeur, std::size_t hauteur) {
  ImageNiveauxGris img(largeur, hauteur);
  const std::size_t n = largeur * hauteur;
  for (std::size_t i = 0; i < n && i < pixels.size(); ++i) img.pixels_[i] = pixels[i];
  return img;
}

void ImageNiveauxGris::remplir(std::uint8_t valeur) {
  std::fill(pixels_.begin(), pixels_.end(), valeur);
}

std::size_t ImageNiveauxGris::nbEncres() const {
  std::size_t n = 0;
  for (const std::uint8_t v : pixels_) {
    if (estEncre(v)) ++n;
  }
  return n;
}

}  // namespace nis
