#include <cmath>
#include <vector>

#include "Harness.h"
#include "core/Appariement.h"

namespace {

nis::Graphe triangleA() {
  std::vector<nis::Sommet> s;
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Bifurcation, 5.0, 0.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 12.0, 0.0});
  return nis::Graphe(s, "A");
}

/// Memes sommets que `triangleA`, ranges dans un autre ordre.
nis::Graphe trianglePermute() {
  std::vector<nis::Sommet> s;
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 12.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  s.push_back(nis::Sommet{nis::TypeMinutie::Bifurcation, 5.0, 0.0, 0.0});
  return nis::Graphe(s, "A permute");
}

}  // namespace

NIS_TEST(appariement_graphes_identiques_deurent_nuls) {
  const nis::Graphe a = triangleA();
  const nis::ResultatAppariement r = nis::Appariement(a, a).calculer();
  NIS_VERIF_PROCHE(r.erreur, 0.0, 1e-9);
  NIS_VERIF(r.exact);
  NIS_VERIF_EQ(r.appariement.size(), 3u);
  NIS_VERIF_EQ(r.appariement[0], 0);
  NIS_VERIF_EQ(r.appariement[1], 1);
  NIS_VERIF_EQ(r.appariement[2], 2);
}

NIS_TEST(appariement_retourne_le_minimum_et_non_le_maximum) {
  // L'ancien code renvoyait le pire cout rencontre (`if (cout1 > cout2) f = cout1`).
  // Avec un permutement, le seul appariement de cout nul est le bon.
  const nis::ResultatAppariement r = nis::Appariement(triangleA(), trianglePermute()).calculer();
  NIS_VERIF_PROCHE(r.erreur, 0.0, 1e-9);
  NIS_VERIF_EQ(r.appariement[0], 1);
  NIS_VERIF_EQ(r.appariement[1], 2);
  NIS_VERIF_EQ(r.appariement[2], 0);

  // Les autres permutations ont un cout strictement positif : le minimum est
  // bien unique ici.
  const std::vector<int> decale = {0, 1, 2};
  const double cout_decale = nis::Appariement::cout(triangleA(), trianglePermute(), decale,
                                                    nis::OptionsAppariement());
  NIS_VERIF(cout_decale > 0.0);
}

NIS_TEST(appariement_penalise_les_desaccords_detiquette) {
  std::vector<nis::Sommet> sa;
  sa.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 4.0, 4.0, 0.0});
  std::vector<nis::Sommet> sb;
  sb.push_back(nis::Sommet{nis::TypeMinutie::Bifurcation, 4.0, 4.0, 0.0});

  nis::OptionsAppariement opt;
  const nis::ResultatAppariement r = nis::Appariement(nis::Graphe(sa), nis::Graphe(sb), opt).calculer();
  NIS_VERIF_PROCHE(r.erreur, opt.penalite_etiquette, 1e-9);
}

NIS_TEST(appariement_sommet_virtuel_coute_penalite) {
  std::vector<nis::Sommet> sa;
  sa.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});
  sa.push_back(nis::Sommet{nis::TypeMinutie::Bifurcation, 30.0, 0.0, 0.0});
  std::vector<nis::Sommet> sb;
  sb.push_back(nis::Sommet{nis::TypeMinutie::Terminaison, 0.0, 0.0, 0.0});

  nis::OptionsAppariement opt;
  const nis::ResultatAppariement r = nis::Appariement(nis::Graphe(sa), nis::Graphe(sb), opt).calculer();
  // Un sommet apparie gratuitement, l'autre paye penalite_sommet + une arete.
  NIS_VERIF_PROCHE(r.erreur, opt.penalite_sommet + opt.penalite_arete, 1e-9);
  int virtuels = 0;
  for (const int j : r.appariement) {
    if (j < 0) ++virtuels;
  }
  NIS_VERIF_EQ(virtuels, 1);
  NIS_VERIF_EQ(r.appariement[0], 0);
}

NIS_TEST(appariement_rejette_les_appariements_invalides) {
  const nis::Graphe a = triangleA();
  const nis::Graphe b = trianglePermute();
  const nis::OptionsAppariement opt;

  NIS_VERIF(!nis::Appariement::estValide(a, b, {0, 0, 2}));   // sommet utilise deux fois
  NIS_VERIF(!nis::Appariement::estValide(a, b, {0, 1}));      // taille incorrecte
  NIS_VERIF(!nis::Appariement::estValide(a, b, {0, 1, 7}));   // hors intervalle
  NIS_VERIF(nis::Appariement::estValide(a, b, {2, 1, 0}));
  NIS_VERIF(nis::Appariement::estValide(a, b, {-1, 1, 0}));

  const double infini = nis::Appariement::cout(a, b, {0, 0, 2}, opt);
  NIS_VERIF(!std::isfinite(infini));
}

NIS_TEST(appariement_graphe_vide) {
  const nis::Graphe vide;
  const nis::ResultatAppariement r = nis::Appariement(vide, triangleA()).calculer();
  NIS_VERIF_EQ(r.appariement.size(), 0u);
  NIS_VERIF_PROCHE(r.erreur, 0.0, 1e-9);
  NIS_VERIF(r.exact);
}

NIS_TEST(appariement_limite_de_noeuds_sur_graphes_distincts) {
  // Cas deterministe (verifie par sonde numerique) ou la solution initiale
  // coute 602.77266732762132 et la recherche complete trouve
  // 553.49702318585719 en 105 noeuds : la solution initiale n'est pas
  // optimale, la recherche doit explorer sous le plafond.
  using T = nis::TypeMinutie;
  const nis::Graphe a(
      {{T::Bifurcation, 6.0, 165.0, 0.0}, {T::Terminaison, 164.0, 32.0, 0.0},
       {T::Bifurcation, 175.0, 129.0, 0.0}, {T::Bifurcation, 41.0, 154.0, 0.0}},
      "a");
  const nis::Graphe b(
      {{T::Terminaison, 163.0, 142.0, 0.0}, {T::Terminaison, 148.0, 42.0, 0.0},
       {T::Terminaison, 98.0, 32.0, 0.0}, {T::Terminaison, 124.0, 149.0, 0.0}},
      "b");

  nis::OptionsAppariement limite;
  limite.max_noeuds = 1;  // plafond : seule la racine est visitee
  const nis::ResultatAppariement coupe = nis::Appariement(a, b, limite).calculer();
  NIS_VERIF(!coupe.exact);
  NIS_VERIF(coupe.noeuds_explores <= 2u);
  NIS_VERIF_PROCHE(coupe.erreur, 602.77266732762132, 1e-6);  // solution initiale

  const nis::ResultatAppariement complet = nis::Appariement(a, b).calculer();
  NIS_VERIF(complet.exact);
  NIS_VERIF_PROCHE(complet.erreur, 553.49702318585719, 1e-6);
  NIS_VERIF(complet.erreur < coupe.erreur);
}

NIS_TEST(appariement_graphes_identiques_reste_exact) {
  // Quand la solution gloutonne est deja optimale (cout nul), la borne coupe
  // immediatement l'exploration : le resultat reste marque exact, car le
  // minimum global est prouve des le depart.
  std::vector<nis::Sommet> s;
  for (int i = 0; i < 7; ++i) {
    s.push_back(nis::Sommet{static_cast<nis::TypeMinutie>(i % 2),
                            static_cast<double>(i * 7), static_cast<double>(i * 3), 0.0});
  }
  const nis::Graphe g(s, "sept sommets");

  nis::OptionsAppariement opt;
  opt.max_noeuds = 1;  // explorerait au plus la racine
  const nis::ResultatAppariement r = nis::Appariement(g, g, opt).calculer();
  NIS_VERIF(r.exact);
  NIS_VERIF_PROCHE(r.erreur, 0.0, 1e-9);
}
