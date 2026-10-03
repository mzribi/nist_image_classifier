// Tests du graphe median : anti-regression du bug historique de
// `graphe_median.cpp` qui recopiait UNE SEULE valeur de poids pour TOUTES les
// aretes (depuis `P[TAB[0][0]][TAB[1][0]]`, avec un risque de lecture hors
// limites quand TAB[0][0] valait -1).
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "Harness.h"
#include "core/GrapheMedian.h"

namespace {

/// Trois sommets : arete (0,1) courte, arete (0,2) longue.  Les poids sont
/// euclidiens, et poids(0,1) != poids(0,2) : une recopie d'une seule valeur
/// pour toutes les aretes se voit immediatement.
nis::Graphe grapheAsymetrique(double echelle) {
  std::vector<nis::Sommet> s;
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 10.0 * echelle, 0.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 30.0 * echelle, 0.0});
  return nis::Graphe(s, "asymetrique");
}

nis::EnsembleGraphes preparer(const std::vector<nis::Graphe>& graphes) {
  nis::EnsembleGraphes ensemble;
  for (const nis::Graphe& g : graphes) ensemble.ajouter(g);
  std::string erreur;
  if (!ensemble.preparer(&erreur)) throw std::runtime_error("preparer: " + erreur);
  return ensemble;
}

}  // namespace

NIS_TEST(median_de_deux_graphes_identiques) {
  const nis::Graphe g = grapheAsymetrique(1.0);
  nis::EnsembleGraphes ensemble = preparer({g, g});
  NIS_VERIF_EQ(ensemble.tailleMedian(), 3u);
  const nis::ResultatMedian r = nis::GrapheMedian().calculer(ensemble);
  NIS_VERIF_EQ(r.graphe.nbSommets(), 3u);
  NIS_VERIF(r.graphe.nom() == "graphe_median");
  NIS_VERIF(r.converge);
  NIS_VERIF(!r.annule);
  NIS_VERIF_PROCHE(r.cout_total, 0.0, 1e-9);
  NIS_VERIF_EQ(r.appariements.size(), 2u);
  NIS_VERIF_PROCHE(r.graphe.poids(0, 1), 10.0, 1e-9);
  NIS_VERIF_PROCHE(r.graphe.poids(0, 2), 30.0, 1e-9);
  NIS_VERIF_PROCHE(r.graphe.poids(1, 2), std::sqrt(100.0 + 900.0), 1e-9);
}

NIS_TEST(median_moyenne_les_poids_case_par_case) {
  // g1 echelle 1 (aretes 10 / 30), g2 echelle 2 (aretes 20 / 60) : les deux
  // appariements possibles donnent des moyennes distinctes par arete.
  const nis::Graphe g1 = grapheAsymetrique(1.0);
  const nis::Graphe g2 = grapheAsymetrique(2.0);
  nis::EnsembleGraphes ensemble = preparer({g1, g2});
  NIS_VERIF_EQ(ensemble.tailleMedian(), 3u);
  const nis::ResultatMedian r = nis::GrapheMedian().calculer(ensemble);
  NIS_VERIF_EQ(r.graphe.nbSommets(), 3u);
  // Anti-regression du bug historique (une seule valeur recopiee pour toutes
  // les aretes, avec lecture hors limites quand TAB[0][0] valait -1) : le
  // point fixe retient ici la graine (g1, cout total minimal rencontre), dont
  // les poids sont les poids REELLEMENT OBSERVES de g1, distincts par arete.
  NIS_VERIF_EQ(r.appariements.size(), 2u);
  NIS_VERIF((r.appariements[0] == std::vector<int>{0, 1, 2}));
  NIS_VERIF((r.appariements[1] == std::vector<int>{1, 0, 2}));
  NIS_VERIF_PROCHE(r.graphe.poids(0, 1), 10.0, 1e-9);
  NIS_VERIF_PROCHE(r.graphe.poids(0, 2), 30.0, 1e-9);
  NIS_VERIF(r.graphe.poids(0, 1) != r.graphe.poids(0, 2));
  // Coherence interne : le cout total annonce est la somme des couts optimaux
  // de chaque graphe au median retourne.
  NIS_VERIF_PROCHE(nis::GrapheMedian::coutTotal({g1, g2}, r.graphe,
                                                nis::OptionsAppariement()),
                   r.cout_total, 1e-6);
  NIS_VERIF(!r.annule);
}

NIS_TEST(median_taille_moyenne_arrondie) {
  // M = moyenne arrondie des tailles : (2 + 4) / 2 = 3.
  std::vector<nis::Sommet> deux;
  deux.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  deux.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 10.0, 0.0, 0.0});
  std::vector<nis::Sommet> quatre = deux;
  quatre.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 10.0, 0.0});
  quatre.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 10.0, 10.0, 0.0});
  nis::EnsembleGraphes ensemble = preparer({nis::Graphe(deux), nis::Graphe(quatre)});
  NIS_VERIF_EQ(ensemble.tailleMedian(), 3u);
  const nis::ResultatMedian r = nis::GrapheMedian().calculer(ensemble);
  NIS_VERIF_EQ(r.graphe.nbSommets(), 3u);
  NIS_VERIF(!r.annule);
}

NIS_TEST(median_ensemble_vide) {
  nis::EnsembleGraphes vide;
  const nis::ResultatMedian r = nis::GrapheMedian().calculer(vide);
  NIS_VERIF_EQ(r.graphe.nbSommets(), 0u);
  NIS_VERIF(r.converge);
}

NIS_TEST(median_honore_l_annulation) {
  const nis::Graphe g = grapheAsymetrique(1.0);
  nis::EnsembleGraphes ensemble = preparer({g, g});
  nis::OptionsMedian opt;
  opt.annulation = [] { return false; };
  const nis::ResultatMedian r = nis::GrapheMedian(opt).calculer(ensemble);
  NIS_VERIF(r.annule);
}
