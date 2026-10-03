#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "Harness.h"
#include "core/Graphe.h"

namespace {

const char* kCheminEssai = "/tmp/nis_essai_graphe.txt";

nis::Graphe grapheExemple() {
  std::vector<nis::Sommet> sommets;
  sommets.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  sommets.push_back(nis::Sommet{nis::TypeMinutie::Bifurcation, 3.0, 4.0, 90.0});
  sommets.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 10.0, 180.0});
  return nis::Graphe(sommets, "exemple");
}

}  // namespace

NIS_TEST(graphe_matrice_est_euclidienne_et_symetrique) {
  const nis::Graphe g = grapheExemple();
  NIS_VERIF_EQ(g.nbSommets(), 3u);
  NIS_VERIF_PROCHE(g.poids(0, 1), 5.0, 1e-9);
  NIS_VERIF_PROCHE(g.poids(1, 0), 5.0, 1e-9);
  NIS_VERIF_PROCHE(g.poids(0, 2), 10.0, 1e-9);
  NIS_VERIF_PROCHE(g.poids(2, 2), 0.0, 1e-9);
  // someAretes = 5 + 10 + sqrt(3^2 + 6^2)
  NIS_VERIF_PROCHE(g.sommeAretes(), 5.0 + 10.0 + std::sqrt(45.0), 1e-9);
}

NIS_TEST(graphe_aller_retour_chaine) {
  const nis::Graphe g = grapheExemple();
  nis::Graphe relu;
  std::string erreur;
  NIS_VERIF(nis::Graphe::depuisChaine(g.versChaine(), &relu, &erreur));
  NIS_VERIF_EQ(relu.nbSommets(), g.nbSommets());
  for (std::size_t i = 0; i < g.nbSommets(); ++i) {
    // Le format legacy ne contient pas l'angle : il n'est pas serialise.
    NIS_VERIF(relu.sommet(i).type == g.sommet(i).type);
    NIS_VERIF_PROCHE(relu.sommet(i).x, g.sommet(i).x, 1e-9);
    NIS_VERIF_PROCHE(relu.sommet(i).y, g.sommet(i).y, 1e-9);
    for (std::size_t j = 0; j < g.nbSommets(); ++j) {
      NIS_VERIF_PROCHE(relu.poids(i, j), g.poids(i, j), 1e-6);
    }
  }
}

NIS_TEST(graphe_accepte_le_format_texte_de_2006) {
  // Fac-simile d'un fichier ecrit par `graphe::ecrire_fichier` : 4 lignes de
  // valeurs entieres puis la matrice.
  const std::string legacy =
      "3\n"
      "0 1 0 \n"
      "10 20 30 \n"
      "10 20 10 \n"
      "0 11 22 \n"
      "11 0 33 \n"
      "22 33 0 \n";
  nis::Graphe g;
  std::string erreur;
  NIS_VERIF(nis::Graphe::depuisChaine(legacy, &g, &erreur));
  NIS_VERIF_EQ(erreur, std::string());
  NIS_VERIF_EQ(g.nbSommets(), 3u);
  NIS_VERIF(g.sommet(1).type == nis::TypeMinutie::Bifurcation);
  NIS_VERIF_PROCHE(g.sommet(2).x, 30.0, 1e-9);
  // Les poids viennent du fichier, pas d'un recalcul euclidien.
  NIS_VERIF_PROCHE(g.poids(0, 1), 11.0, 1e-9);
  NIS_VERIF_PROCHE(g.poids(1, 2), 33.0, 1e-9);
}

NIS_TEST(graphe_rejette_les_fichiers_invalides) {
  nis::Graphe g;
  std::string erreur;

  NIS_VERIF(!nis::Graphe::depuisChaine("", &g, &erreur));
  NIS_VERIF(!erreur.empty());

  erreur.clear();
  NIS_VERIF(!nis::Graphe::depuisChaine("2\n0 1\n1 2\n", &g, &erreur));
  NIS_VERIF(!erreur.empty());

  erreur.clear();
  NIS_VERIF(!nis::Graphe::depuisChaine("1\n5\n1\n1\n0\n", &g, &erreur));
  NIS_VERIF(erreur.find("etiquette") != std::string::npos);

  erreur.clear();
  NIS_VERIF(!nis::Graphe::depuisChaine("-3\n", &g, &erreur));

  // Fichier absent : erreur, et pas d'acces memoire.
  erreur.clear();
  NIS_VERIF(!nis::Graphe::depuisFichier("/tmp/nis_absent_42.txt", &g, &erreur));
  NIS_VERIF(!erreur.empty());
}

NIS_TEST(graphe_aller_retour_fichier) {
  const nis::Graphe g = grapheExemple();
  std::string erreur;
  NIS_VERIF(g.ecrireFichier(kCheminEssai, &erreur));
  NIS_VERIF_EQ(erreur, std::string());

  // Le fichier commence par le nombre de sommets, comme en 2006.
  {
    std::ifstream flux(kCheminEssai);
    std::string premiere_ligne;
    std::getline(flux, premiere_ligne);
    NIS_VERIF_EQ(premiere_ligne, std::string("3"));
  }

  nis::Graphe relu;
  NIS_VERIF(nis::Graphe::depuisFichier(kCheminEssai, &relu, &erreur));
  NIS_VERIF_EQ(relu.nom(), std::string(kCheminEssai));
  NIS_VERIF_EQ(relu.nbSommets(), 3u);
  NIS_VERIF_PROCHE(relu.poids(0, 1), 5.0, 1e-6);
  std::remove(kCheminEssai);
}

NIS_TEST(graphe_refuse_une_matrice_incoherente) {
  nis::Graphe g = grapheExemple();
  nis::Graphe::Matrice mauvaise(2, std::vector<double>(2, 1.0));
  NIS_VERIF(!g.setMatrice(mauvaise));
  NIS_VERIF_PROCHE(g.poids(0, 1), 5.0, 1e-9);  // l'ancienne matrice est conservee

  nis::Graphe::Matrice bonne(3, std::vector<double>(3, 0.0));
  bonne[0][1] = bonne[1][0] = 7.0;
  NIS_VERIF(g.setMatrice(bonne));
  NIS_VERIF_PROCHE(g.poids(0, 1), 7.0, 1e-9);
}

NIS_TEST(graphe_vide_est_autorise) {
  nis::Graphe g;
  NIS_VERIF_EQ(g.nbSommets(), 0u);
  NIS_VERIF_EQ(g.sommeAretes(), 0.0);
  nis::Graphe relu;
  std::string erreur;
  NIS_VERIF(nis::Graphe::depuisChaine(g.versChaine(), &relu, &erreur));
  NIS_VERIF_EQ(relu.nbSommets(), 0u);
}
