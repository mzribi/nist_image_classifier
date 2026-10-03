// Tests de la chaine image : binarisation, suppression du fond accroche,
// amincissement de Zhang-Suen et extraction des minuties.
// Anti-regressions visees : lecture hors limites de 2006 (`image[i+1][j+1]`
// sur les bords), `while(1)` sans garantie d'arret de `amincissement1/2/3`,
// suppression dependant de l'ordre de parcours, zone [80..200] codee en dur
// de `extraction_bifurcation`, plafond de 5 minuties de `extraction_sommet`.
#include <cmath>
#include <cstddef>
#include <vector>

#include "Harness.h"
#include "core/PipelineImage.h"

namespace {

nis::ImageNiveauxGris imageVierge(std::size_t l, std::size_t h) {
  return nis::ImageNiveauxGris(l, h, nis::ImageNiveauxGris::Fond);
}

void tracerLigne(nis::ImageNiveauxGris* img, int x0, int y0, int x1, int y1) {
  int dx = (x1 > x0) ? 1 : -1;
  int dy = (y1 > y0) ? 1 : -1;
  int x = x0, y = y0;
  img->set(static_cast<std::size_t>(x), static_cast<std::size_t>(y),
           nis::ImageNiveauxGris::Encre);
  while (x != x1 || y != y1) {
    if (x != x1) x += dx;
    if (y != y1) y += dy;
    img->set(static_cast<std::size_t>(x), static_cast<std::size_t>(y),
             nis::ImageNiveauxGris::Encre);
  }
}

/// Un « T » : barre horizontale (2,4)-(17,4), pied (10,4)-(10,15), image 20x20.
nis::ImageNiveauxGris imageEnT() {
  nis::ImageNiveauxGris img = imageVierge(20, 20);
  tracerLigne(&img, 2, 4, 17, 4);
  tracerLigne(&img, 10, 4, 10, 15);
  return img;
}

bool estDansBloc2x2(const nis::ImageNiveauxGris& img, std::size_t x, std::size_t y) {
  const std::size_t l = img.largeur(), h = img.hauteur();
  for (int dy = -1; dy <= 0; ++dy) {
    for (int dx = -1; dx <= 0; ++dx) {
      const int x0 = static_cast<int>(x) + dx, y0 = static_cast<int>(y) + dy;
      if (x0 < 0 || y0 < 0) continue;
      const std::size_t ux = static_cast<std::size_t>(x0);
      const std::size_t uy = static_cast<std::size_t>(y0);
      if (ux + 1 >= l || uy + 1 >= h) continue;
      if (img.estEncre(ux, uy) && img.estEncre(ux + 1, uy) && img.estEncre(ux, uy + 1) &&
          img.estEncre(ux + 1, uy + 1))
        return true;
    }
  }
  return false;
}

int compterVoisinsEncre(const nis::ImageNiveauxGris& img, std::size_t x, std::size_t y) {
  int n = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      const int xx = static_cast<int>(x) + dx, yy = static_cast<int>(y) + dy;
      if (xx < 0 || yy < 0) continue;
      if (img.estEncre(static_cast<std::size_t>(xx), static_cast<std::size_t>(yy))) ++n;
    }
  }
  return n;
}

nis::OptionsPipeline optionsTest() {
  nis::OptionsPipeline o;
  o.max_passes = 200;
  o.distance_minimale = 0;  // pas de fusion : on compte les vraies minuties
  return o;
}

}  // namespace

NIS_TEST(pipeline_binarisation_seuil) {
  nis::ImageNiveauxGris img(4, 1);
  img(0, 0) = 0; img(1, 0) = 127; img(2, 0) = 128; img(3, 0) = 255;
  const nis::ImageNiveauxGris b = nis::pipeline::binariser(img, 128);
  NIS_VERIF(b.estEncre(0, 0));
  NIS_VERIF(b.estEncre(1, 0));
  NIS_VERIF(!b.estEncre(2, 0));
  NIS_VERIF(!b.estEncre(3, 0));
}

NIS_TEST(pipeline_eliminer_fond_rognent_les_bords) {
  // Gris 200 colle a gauche et a droite du trait : seule la bande du trait
  // doit survivre (encodee ici en 0 pour rester de l'encre).
  nis::ImageNiveauxGris img(20, 3);
  for (std::size_t x = 0; x < 20; ++x) { img(x, 1) = 200; }
  img(9, 1) = 0; img(10, 1) = 0;
  const nis::ImageNiveauxGris r = nis::pipeline::eliminerFond(img);
  NIS_VERIF(r.estEncre(9, 1));
  NIS_VERIF(r.estEncre(10, 1));
  NIS_VERIF(!r.estEncre(0, 1));
  NIS_VERIF(!r.estEncre(19, 1));
  NIS_VERIF(!r.estEncre(5, 0));  // ligne sans trait : tout fond
}

NIS_TEST(pipeline_amincissement_converge_sans_bloc) {
  // Barre epaisse 5x20 : le squelette doit converger vers un trait fin.
  nis::ImageNiveauxGris img = imageVierge(20, 20);
  for (std::size_t y = 8; y <= 12; ++y)
    for (std::size_t x = 2; x <= 17; ++x) img(x, y) = nis::ImageNiveauxGris::Encre;
  const std::size_t avant = img.nbEncres();
  nis::OptionsPipeline o = optionsTest();
  const nis::EtapePipeline e = nis::pipeline::amincir(&img, o);
  NIS_VERIF(e.converge);
  NIS_VERIF(img.nbEncres() < avant);
  NIS_VERIF(img.nbEncres() > 0u);
  for (std::size_t y = 0; y < img.hauteur(); ++y)
    for (std::size_t x = 0; x < img.largeur(); ++x)
      if (img.estEncre(x, y)) {
        NIS_VERIF(!estDansBloc2x2(img, x, y));
        NIS_VERIF(compterVoisinsEncre(img, x, y) <= 2);
      }
}

NIS_TEST(pipeline_extrait_terminaisons_et_bifurcation_du_T) {
  // L'epaisseur d'un pixel de la barre fait emerger la jonction sur 3 pixels
  // adjacents (cote pied + cotes barre) : on exige donc les 3 terminaisons
  // attendues et AU MOINS une bifurcation placee sur la jonction (x = 9..11,
  // y = 4), plutot qu'un compte fragile de bifurcations.
  nis::ImageNiveauxGris img = imageEnT();
  nis::OptionsPipeline o = optionsTest();
  const nis::EtapePipeline e = nis::pipeline::amincir(&img, o);
  NIS_VERIF(e.converge);
  const std::vector<nis::Sommet> m = nis::pipeline::extraireMinuties(img, o);
  int term = 0, bif = 0;
  bool jonction_vue = false;
  for (const nis::Sommet& s : m) {
    if (s.type == nis::TypeMinutie::Terminaison) ++term;
    else {
      ++bif;
      if (s.x >= 9.0 && s.x <= 11.0 && std::fabs(s.y - 4.0) < 1e-9) jonction_vue = true;
    }
  }
  NIS_VERIF_EQ(term, 3);
  NIS_VERIF(bif >= 1);
  NIS_VERIF(jonction_vue);
}

NIS_TEST(pipeline_poids_sont_euclidiens) {
  std::vector<nis::Sommet> s;
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 3.0, 4.0, 0.0});
  const nis::Graphe g = nis::pipeline::construireGraphe(s, "t");
  NIS_VERIF_EQ(g.nbSommets(), 2u);
  NIS_VERIF_PROCHE(g.poids(0, 1), 5.0, 1e-9);
  NIS_VERIF(g.nom() == "t");
}

NIS_TEST(pipeline_chaine_complete) {
  const nis::ImageNiveauxGris img = imageEnT();
  const nis::pipeline::ResultatChaine r = nis::pipeline::chaineComplete(img, optionsTest());
  NIS_VERIF(r.amincissement.converge);
  NIS_VERIF(r.minuties.size() >= 4u);  // 3 terminaisons + jonction(s)
  NIS_VERIF_EQ(r.graphe.nbSommets(), r.minuties.size());
  NIS_VERIF(r.graphe.poids(0, 1) > 0.0);
}
