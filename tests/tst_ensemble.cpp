// Tests d'EnsembleGraphes : taille mediane M, fenetres de colonnes,
// etiquettes etendues (EXT_ES) et alphabet reduit.
// Anti-regressions : `graphe_associe()` ne remettait jamais l'etat a zero
// (deuxieme appel faux), k-moyennes avec 2*M centrois sur des fenetres
// compressees donc possiblement vides.
#include <string>
#include <vector>

#include "Harness.h"
#include "core/EnsembleGraphes.h"

namespace {

nis::Graphe ligne(std::size_t n, nis::TypeMinutie type) {
  std::vector<nis::Sommet> s;
  for (std::size_t i = 0; i < n; ++i)
    s.push_back(nis::Sommet{type, static_cast<double>(10 * i), 0.0, 0.0});
  return nis::Graphe(s);
}

}  // namespace

NIS_TEST(ensemble_taille_mediane_et_fenetres) {
  // Tailles 2 et 4 : M = lround(3.0) = 3, fenetres max(3,2)=3 et max(3,4)=4.
  nis::EnsembleGraphes e;
  e.ajouter(ligne(2, nis::TypeMinutie::Terminaison));
  e.ajouter(ligne(4, nis::TypeMinutie::Bifurcation));
  std::string erreur;
  NIS_VERIF(e.preparer(&erreur));
  NIS_VERIF(e.pret());
  NIS_VERIF_EQ(e.tailleMedian(), 3u);
  NIS_VERIF_EQ(e.nbColonnes(), 7u);
  NIS_VERIF_EQ(e.colonneInitiale(0), 0u);
  NIS_VERIF_EQ(e.largeurFenetre(0), 3u);
  NIS_VERIF_EQ(e.colonneInitiale(1), 3u);
  NIS_VERIF_EQ(e.largeurFenetre(1), 4u);
  NIS_VERIF_EQ(e.colonneGlobale(1, 2), 5u);
  NIS_VERIF_EQ(e.etiquettes().size(), 6u);
  NIS_VERIF_EQ(e.etiquettesEtendues().size(), 7u);
  NIS_VERIF_EQ(e.etiquettesEtendues()[2], nis::EnsembleGraphes::SommetVirtuel);
  NIS_VERIF(e.colonneReelle(0));
  NIS_VERIF(!e.colonneReelle(2));  // colonne virtuelle du premier graphe
  NIS_VERIF(e.colonneReelle(6));
  std::size_t g = 99, locale = 99;
  NIS_VERIF(e.colonneLocale(5, &g, &locale));
  NIS_VERIF_EQ(g, 1u);
  NIS_VERIF_EQ(locale, 2u);
  NIS_VERIF(!e.colonneLocale(7, &g, &locale));
  NIS_VERIF_EQ(e.etiquetteColonne(2), nis::EnsembleGraphes::SommetVirtuel);
  NIS_VERIF(!e.alphabet().empty());
}

NIS_TEST(ensemble_est_reproductible_et_sans_etat_cache) {
  // Deux preparations successives donnent le meme resultat (2006 : le
  // deuxieme appel de `graphe_associe()` etait faux).
  nis::EnsembleGraphes e;
  e.ajouter(ligne(3, nis::TypeMinutie::Terminaison));
  e.ajouter(ligne(3, nis::TypeMinutie::Bifurcation));
  NIS_VERIF(e.preparer(nullptr));
  const std::vector<int> premieres = e.etiquettesEtendues();
  NIS_VERIF(e.preparer(nullptr));
  NIS_VERIF_EQ(e.etiquettesEtendues().size(), premieres.size());
  NIS_VERIF(e.etiquettesEtendues() == premieres);
}

NIS_TEST(ensemble_vide_est_signale) {
  nis::EnsembleGraphes e;
  std::string erreur;
  NIS_VERIF(!e.preparer(&erreur));
  NIS_VERIF(!erreur.empty());
  NIS_VERIF(!e.pret());
  nis::EnsembleGraphes que_vides;
  que_vides.ajouter(nis::Graphe());
  NIS_VERIF(!que_vides.preparer(&erreur));
  NIS_VERIF(!erreur.empty());
}
