#include <algorithm>
#include <vector>

#include "Harness.h"
#include "core/KMeans1D.h"

NIS_TEST(kmeans_separe_les_deux_etiquettes) {
  const std::vector<int> valeurs = {0, 0, 0, 1, 1};
  const nis::ResultatKMeans r = nis::KMeans1D(valeurs).lancer(2);

  NIS_VERIF_EQ(r.kEffectif, 2u);
  NIS_VERIF(r.converge);
  NIS_VERIF_EQ(r.centrois.size(), 2u);
  // Bug 2006 : la moyenne entiere `1 / 2` ecrasait les centrois vers 0.
  NIS_VERIF_EQ(r.centrois[0], 0);
  NIS_VERIF_EQ(r.centrois[1], 1);
  NIS_VERIF_EQ(r.etiquettes.size(), valeurs.size());
  NIS_VERIF_EQ(r.etiquettes[0], 0);
  NIS_VERIF_EQ(r.etiquettes[3], 1);
  NIS_VERIF_EQ(r.etiquettes[4], 1);
}

NIS_TEST(kmeans_ne_depasse_jamais_le_nombre_de_valeurs_distinctes) {
  const std::vector<int> valeurs = {7, 7, 7, 7};
  const nis::ResultatKMeans r = nis::KMeans1D(valeurs).lancer(4, 100);
  NIS_VERIF_EQ(r.kDemande, 4u);
  NIS_VERIF_EQ(r.kEffectif, 1u);
  NIS_VERIF_EQ(r.centrois.size(), 1u);
  NIS_VERIF_EQ(r.centrois[0], 7);
  NIS_VERIF(r.converge);
  NIS_VERIF(r.iterations <= 100u);
}

NIS_TEST(kmeans_sarrete_meme_sur_un_cas_degenerescent) {
  // Deux centrois a egale distance peuvent flotter : la borne doit couper.
  const std::vector<int> valeurs = {0, 10};
  const nis::ResultatKMeans r = nis::KMeans1D(valeurs).lancer(5, 7);
  NIS_VERIF(r.iterations <= 7u);
  NIS_VERIF_EQ(r.etiquettes.size(), 2u);
}

NIS_TEST(kmeans_sur_une_entree_vide) {
  const nis::ResultatKMeans r = nis::KMeans1D(std::vector<int>{}).lancer(3);
  NIS_VERIF_EQ(r.centrois.size(), 0u);
  NIS_VERIF_EQ(r.etiquettes.size(), 0u);
  NIS_VERIF_EQ(r.kEffectif, 0u);
}

NIS_TEST(kmeans_est_deterministe) {
  const std::vector<int> valeurs = {0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0};
  const nis::ResultatKMeans a = nis::KMeans1D(valeurs).lancer(2);
  const nis::ResultatKMeans b = nis::KMeans1D(valeurs).lancer(2);
  NIS_VERIF(a.centrois == b.centrois);
  NIS_VERIF(a.etiquettes == b.etiquettes);
}

NIS_TEST(kmeans_repart_des_niveaux_plus_numerosos) {
  std::vector<int> valeurs;
  for (int v = 0; v < 30; ++v) valeurs.push_back(v % 5);
  const nis::ResultatKMeans r = nis::KMeans1D(valeurs).lancer(5);
  NIS_VERIF_EQ(r.kEffectif, 5u);
  NIS_VERIF_EQ(r.centrois.size(), 5u);
  // Chaque centrois doit etre une des valeurs observes, et les centrois sont
  // tries dans l'ordre croissant (initialisation repartie sur l'alphabet trie).
  NIS_VERIF(std::is_sorted(r.centrois.begin(), r.centrois.end()));
  for (const int c : r.centrois) {
    NIS_VERIF(std::find(valeurs.begin(), valeurs.end(), c) != valeurs.end());
  }
}
