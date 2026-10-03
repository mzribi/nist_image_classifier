#include "core/PipelineImage.h"

#include <algorithm>
#include <cmath>

namespace nis {
namespace pipeline {
namespace {

struct Decalage {
  int dx;
  int dy;
};

// Voisins P2..P9 dans l'ordre de Zhang-Suen : N, NE, E, SE, S, SW, W, NW.
constexpr Decalage kVoisins[8] = {{0, -1}, {1, -1}, {1, 0},  {1, 1},
                                  {0, 1},  {-1, 1}, {-1, 0}, {-1, -1}};

/// Encre avec gestion des bords : hors image = fond.  L'ancien code lisait
/// `image[i+1][j+1]` sur la derniere ligne/lacolonne.
bool encre(const ImageNiveauxGris& img, int x, int y) {
  if (x < 0 || y < 0) return false;
  const std::size_t ux = static_cast<std::size_t>(x);
  const std::size_t uy = static_cast<std::size_t>(y);
  if (ux >= img.largeur() || uy >= img.hauteur()) return false;
  return ImageNiveauxGris::estEncre(img.au(ux, uy));
}

int valeur(const ImageNiveauxGris& img, int x, int y) { return encre(img, x, y) ? 1 : 0; }

int compterVoisins(const ImageNiveauxGris& img, int x, int y) {
  int n = 0;
  for (const Decalage& v : kVoisins) {
    if (encre(img, x + v.dx, y + v.dy)) ++n;
  }
  return n;
}

/// A(P) : nombre de transitions 0 -> 1 dans le cycle P2, P3, ... P9, P2.
int transitions(const ImageNiveauxGris& img, int x, int y) {
  int sequence[9];
  for (int k = 0; k < 8; ++k) sequence[k] = valeur(img, x + kVoisins[k].dx, y + kVoisins[k].dy);
  sequence[8] = sequence[0];
  int n = 0;
  for (int k = 0; k < 8; ++k) {
    if (sequence[k] == 0 && sequence[k + 1] == 1) ++n;
  }
  return n;
}

/// Une passe de Zhang-Suen.  Les pixels a supprimer sont marques puis retires
/// apres la boucle : le resultat ne depend plus de l'ordre de parcours.
bool passerAmincissement(const ImageNiveauxGris& src, ImageNiveauxGris* dst, bool premiere_etape) {
  bool modifie = false;
  std::vector<std::uint8_t> marquage(src.taille(), 0);
  const std::size_t L = src.largeur();
  const std::size_t H = src.hauteur();
  for (std::size_t y = 0; y < H; ++y) {
    for (std::size_t x = 0; x < L; ++x) {
      const int ix = static_cast<int>(x);
      const int iy = static_cast<int>(y);
      if (!encre(src, ix, iy)) continue;
      const int B = compterVoisins(src, ix, iy);
      if (B < 2 || B > 6) continue;
      if (transitions(src, ix, iy) != 1) continue;
      const int P2 = valeur(src, ix, iy - 1);
      const int P4 = valeur(src, ix + 1, iy);
      const int P6 = valeur(src, ix, iy + 1);
      const int P8 = valeur(src, ix - 1, iy);
      const bool condition =
          premiere_etape ? (P2 * P4 * P6 == 0 && P4 * P6 * P8 == 0)
                         : (P2 * P4 * P8 == 0 && P2 * P6 * P8 == 0);
      if (condition) marquage[y * L + x] = 1;
    }
  }
  for (std::size_t y = 0; y < H; ++y) {
    for (std::size_t x = 0; x < L; ++x) {
      if (marquage[y * L + x]) {
        (*dst)(x, y) = ImageNiveauxGris::Fond;
        modifie = true;
      }
    }
  }
  return modifie;
}

/// Vrai si le pixel appartient a un bloc 2x2 entierement d'encre : sur un
/// squelette propre d'un pixel de large cela n'arrive pas, la structure trahit
/// un epaississement local ou un reste de bruit.
bool dansBlocCompact(const ImageNiveauxGris& img, int x, int y) {
  for (int dy = -1; dy <= 0; ++dy) {
    for (int dx = -1; dx <= 0; ++dx) {
      if (encre(img, x + dx, y + dy) && encre(img, x + dx + 1, y + dy) &&
          encre(img, x + dx, y + dy + 1) && encre(img, x + dx + 1, y + dy + 1)) {
        return true;
      }
    }
  }
  return false;
}

/// Angle en degrees, origine a l'est, sens trigonometrique (l'axe y de l'image
/// est dirige vers le bas, d'ou le signe).
double angleVoisins(const ImageNiveauxGris& img, int x, int y) {
  double sx = 0.0, sy = 0.0;
  for (const Decalage& v : kVoisins) {
    if (encre(img, x + v.dx, y + v.dy)) {
      sx += static_cast<double>(v.dx);
      sy -= static_cast<double>(v.dy);
    }
  }
  double degres = std::atan2(sy, sx) * 180.0 / 3.14159265358979323846;
  if (degres < 0.0) degres += 360.0;
  return degres;
}

}  // namespace

ImageNiveauxGris binariser(const ImageNiveauxGris& src, int seuil) {
  ImageNiveauxGris out(src.largeur(), src.hauteur());
  const int s = std::max(1, std::min(255, seuil));
  for (std::size_t y = 0; y < src.hauteur(); ++y) {
    for (std::size_t x = 0; x < src.largeur(); ++x) {
      out(x, y) = (src.au(x, y) < static_cast<std::uint8_t>(s)) ? ImageNiveauxGris::Encre
                                                                : ImageNiveauxGris::Fond;
    }
  }
  return out;
}

ImageNiveauxGris eliminerFond(const ImageNiveauxGris& src) {
  ImageNiveauxGris out = src;
  for (std::size_t y = 0; y < src.hauteur(); ++y) {
    std::size_t premier = src.largeur();
    std::size_t dernier = 0;
    bool trouve = false;
    for (std::size_t x = 0; x < src.largeur(); ++x) {
      if (encre(src, static_cast<int>(x), static_cast<int>(y))) {
        if (!trouve) {
          premier = x;
          trouve = true;
        }
        dernier = x;
      }
    }
    if (!trouve) {
      // Ligne sans trait : elle devient entierement fond.
      for (std::size_t x = 0; x < src.largeur(); ++x) out(x, y) = ImageNiveauxGris::Fond;
      continue;
    }
    for (std::size_t x = 0; x < premier; ++x) out(x, y) = ImageNiveauxGris::Fond;
    for (std::size_t x = dernier + 1; x < src.largeur(); ++x) out(x, y) = ImageNiveauxGris::Fond;
  }
  return out;
}

EtapePipeline amincir(ImageNiveauxGris* image, const OptionsPipeline& opt) {
  EtapePipeline etape;
  if (image == nullptr || image->vide()) return etape;

  ImageNiveauxGris courant = *image;
  ImageNiveauxGris tampon;
  const std::size_t max_passes = std::max<std::size_t>(1, opt.max_passes);
  for (std::size_t p = 1; p <= max_passes; ++p) {
    etape.passes = p;

    tampon = courant;
    const bool m1 = passerAmincissement(courant, &tampon, true);
    if (m1) courant = tampon;

    tampon = courant;
    const bool m2 = passerAmincissement(courant, &tampon, false);
    if (m2) courant = tampon;

    if (!m1 && !m2) {
      etape.converge = true;
      break;
    }
  }
  *image = std::move(courant);
  return etape;
}

std::vector<Sommet> extraireMinuties(const ImageNiveauxGris& squelette,
                                     const OptionsPipeline& opt) {
  std::vector<Sommet> minuties;
  const double d_min = static_cast<double>(opt.distance_minimale);
  const double d_min_carre = d_min * d_min;

  for (std::size_t y = 0; y < squelette.hauteur(); ++y) {
    for (std::size_t x = 0; x < squelette.largeur(); ++x) {
      const int ix = static_cast<int>(x);
      const int iy = static_cast<int>(y);
      if (!encre(squelette, ix, iy)) continue;

      const int B = compterVoisins(squelette, ix, iy);
      TypeMinutie type;
      if (B == 1) {
        type = TypeMinutie::Terminaison;
      } else if (B == 3 && !dansBlocCompact(squelette, ix, iy)) {
        type = TypeMinutie::Bifurcation;
      } else {
        continue;
      }

      if (opt.distance_minimale > 0) {
        bool trop_proche = false;
        for (const Sommet& s : minuties) {
          if (s.type != type) continue;
          const double dx = static_cast<double>(x) - s.x;
          const double dy = static_cast<double>(y) - s.y;
          if (dx * dx + dy * dy <= d_min_carre) {
            trop_proche = true;
            break;
          }
        }
        if (trop_proche) continue;
      }

      Sommet s;
      s.type = type;
      s.x = static_cast<double>(x);
      s.y = static_cast<double>(y);
      s.angle = angleVoisins(squelette, ix, iy);
      minuties.push_back(s);

      if (opt.max_sommets != 0 && minuties.size() >= opt.max_sommets) return minuties;
    }
  }
  return minuties;
}

Graphe construireGraphe(const std::vector<Sommet>& sommets, const std::string& nom) {
  return Graphe(sommets, nom);
}

ResultatChaine chaineComplete(const ImageNiveauxGris& src, const OptionsPipeline& opt) {
  ResultatChaine r;
  r.binarisee = binariser(src, opt.seuil);
  r.sansFond = eliminerFond(r.binarisee);
  r.squelette = r.sansFond;
  r.amincissement = amincir(&r.squelette, opt);
  r.minuties = extraireMinuties(r.squelette, opt);
  r.graphe = construireGraphe(r.minuties, "graphe");
  return r;
}

}  // namespace pipeline
}  // namespace nis
