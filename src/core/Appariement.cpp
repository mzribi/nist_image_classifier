#include "core/Appariement.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nis {
namespace {

constexpr double CoutInvalide = std::numeric_limits<double>::infinity();

/// Profil geometrique d'un sommet : la liste triee de ses distances aux autres
/// sommets du graphe.  Deux sommets de graphes differentis ayant des profils
/// proches sont de bons candidats a l'appariement ; c'est l'heuristique qui
/// remplace le tirage aleatoire jamais initialise de 2006.
std::vector<std::vector<double>> profils(const Graphe& g) {
  const std::size_t n = g.nbSommets();
  std::vector<std::vector<double>> p(n);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      if (i != j) p[i].push_back(g.poids(i, j));
    }
    std::sort(p[i].begin(), p[i].end());
  }
  return p;
}

double scoreCandidat(const std::vector<double>& pa, const std::vector<double>& pb,
                     const Sommet& sa, const Sommet& sb, const OptionsAppariement& opt) {
  const std::size_t m = std::min(pa.size(), pb.size());
  double ecart = 0.0;
  for (std::size_t t = 0; t < m; ++t) ecart += std::fabs(pa[t] - pb[t]);
  if (m > 0) ecart /= static_cast<double>(m);
  return opt.penalite_etiquette * std::fabs(differenceType(sa.type, sb.type)) + ecart;
}

struct Etat {
  const Graphe* a{nullptr};
  const Graphe* b{nullptr};
  const OptionsAppariement* opt{nullptr};
  std::size_t nA{0};
  std::size_t nB{0};
  std::vector<int> ap;                        // -2 = pas encore affecte
  std::vector<char> utilise;                  // nB
  std::vector<std::vector<int>> candidats;    // par sommet de A, valeurs j triees
  std::size_t noeuds{0};
  std::size_t plafond{0};
  bool limite{false};
  std::vector<int> meilleur;
  double meilleur_cout{CoutInvalide};
};

/// Surcout ajoute par l'affectation du sommet `i` de A.  Les paires (i', i) avec
/// i' < i ne sont comptees qu'ici, donc exactement une fois.
double increment(Etat& e, std::size_t i, int j) {
  double inc = 0.0;
  if (j < 0) {
    inc = e.opt->penalite_sommet + static_cast<double>(i) * e.opt->penalite_arete;
    return inc;
  }
  inc = e.opt->penalite_etiquette *
        std::fabs(differenceType(e.a->sommet(i).type, e.b->sommet(static_cast<std::size_t>(j)).type));
  for (std::size_t i2 = 0; i2 < i; ++i2) {
    const int j2 = e.ap[i2];
    if (j2 < 0) {
      inc += e.opt->penalite_arete;
    } else {
      inc += std::fabs(e.a->poids(i2, i) -
                       e.b->poids(static_cast<std::size_t>(j2), static_cast<std::size_t>(j)));
    }
  }
  return inc;
}

void explorer(Etat& e, std::size_t i, double cout_courant) {
  if (e.limite) return;
  ++e.noeuds;
  if (e.noeuds > e.plafond) {
    e.limite = true;
    return;
  }
  if (cout_courant >= e.meilleur_cout) return;  // borne admissible : tout cout partiel <= total

  if (i == e.nA) {
    e.meilleur_cout = cout_courant;
    e.meilleur = e.ap;
    return;
  }

  for (const int j : e.candidats[i]) {
    if (e.utilise[static_cast<std::size_t>(j)]) continue;
    const double inc = increment(e, i, j);
    if (cout_courant + inc >= e.meilleur_cout) continue;
    e.ap[i] = j;
    e.utilise[static_cast<std::size_t>(j)] = 1;
    explorer(e, i + 1, cout_courant + inc);
    e.utilise[static_cast<std::size_t>(j)] = 0;
    e.ap[i] = -2;
    if (e.limite) return;
  }

  // Option « sommet apparie a rien », evaluee en dernier : elle n'est retenue
  // que si elle reste strictement meilleure.
  const double inc_virtuel = increment(e, i, -1);
  if (cout_courant + inc_virtuel < e.meilleur_cout) {
    e.ap[i] = -1;
    explorer(e, i + 1, cout_courant + inc_virtuel);
    e.ap[i] = -2;
  }
}

}  // namespace

Appariement::Appariement(const Graphe& a, const Graphe& b, OptionsAppariement options)
    : a_(a), b_(b), options_(options) {}

bool Appariement::estValide(const Graphe& a, const Graphe& b,
                           const std::vector<int>& appariement) {
  if (appariement.size() != a.nbSommets()) return false;
  std::vector<char> vus(b.nbSommets(), 0);
  for (const int j : appariement) {
    // -1 designe le sommet virtuel : aucun sommet de b ne lui est associe.
    // Le test doit se faire AVANT tout cast vers size_t, dont -1 deviendrait
    // la plus grande valeur possible.
    if (j == -1) continue;
    if (j < 0 || static_cast<std::size_t>(j) >= b.nbSommets()) return false;
    if (vus[static_cast<std::size_t>(j)]) return false;
    vus[static_cast<std::size_t>(j)] = 1;
  }
  return true;
}

double Appariement::cout(const Graphe& a, const Graphe& b,
                        const std::vector<int>& appariement,
                        const OptionsAppariement& options) {
  if (!estValide(a, b, appariement)) return CoutInvalide;

  double total = 0.0;
  const std::size_t n = a.nbSommets();
  for (std::size_t i = 0; i < n; ++i) {
    const int j = appariement[i];
    if (j < 0) {
      total += options.penalite_sommet;
    } else {
      total += options.penalite_etiquette *
               std::fabs(differenceType(a.sommet(i).type, b.sommet(static_cast<std::size_t>(j)).type));
    }
  }
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t k = i + 1; k < n; ++k) {
      const int ji = appariement[i];
      const int jk = appariement[k];
      if (ji < 0 || jk < 0) {
        total += options.penalite_arete;
      } else {
        total += std::fabs(a.poids(i, k) -
                           b.poids(static_cast<std::size_t>(ji), static_cast<std::size_t>(jk)));
      }
    }
  }
  return total;
}

ResultatAppariement Appariement::calculer() const {
  ResultatAppariement res;
  const std::size_t nA = a_.nbSommets();
  const std::size_t nB = b_.nbSommets();

  if (nA == 0) {
    res.exact = true;
    res.erreur = 0.0;
    return res;
  }

  Etat e;
  e.a = &a_;
  e.b = &b_;
  e.opt = &options_;
  e.nA = nA;
  e.nB = nB;
  e.ap.assign(nA, -2);
  e.utilise.assign(nB, 0);
  e.plafond = std::max<std::size_t>(1, options_.max_noeuds);

  // Ordre des candidats : etiquettes identiques d'abord, puis profils geometriques
  // les plus proches.  Un bon point de depart est essentiel pour que la
  // separation et bornes elimine des branches.
  const std::vector<std::vector<double>> pa = profils(a_);
  const std::vector<std::vector<double>> pb = profils(b_);
  e.candidats.assign(nA, {});
  for (std::size_t i = 0; i < nA; ++i) {
    e.candidats[i].reserve(nB);
    for (std::size_t j = 0; j < nB; ++j) e.candidats[i].push_back(static_cast<int>(j));
    std::stable_sort(e.candidats[i].begin(), e.candidats[i].end(),
                     [&](int x, int y) {
                       return scoreCandidat(pa[i], pb[static_cast<std::size_t>(x)], a_.sommet(i),
                                            b_.sommet(static_cast<std::size_t>(x)), options_) <
                              scoreCandidat(pa[i], pb[static_cast<std::size_t>(y)], a_.sommet(i),
                                            b_.sommet(static_cast<std::size_t>(y)), options_);
                     });
  }

  // Première borne superieure : le meilleur appariement monocandidat trouvé
  // en rangeant les sommets de A dans l'ordre.  Appelée « gloutonne » dans
  // les tests, elle n'est pas forcément optimale (les croisements de coûts
  // d'aretes peuvent faire mieux), mais elle calibre la coupe par borne.
  {
    std::vector<int> glouton(nA, -1);
    std::vector<char> pris(nB, 0);
    for (std::size_t i = 0; i < nA; ++i) {
      for (const int j : e.candidats[i]) {
        if (!pris[static_cast<std::size_t>(j)]) {
          glouton[i] = j;
          pris[static_cast<std::size_t>(j)] = 1;
          break;
        }
      }
    }
    e.meilleur = glouton;
    e.meilleur_cout = cout(a_, b_, glouton, options_);
  }

  explorer(e, 0, 0.0);

  res.appariement = e.meilleur;
  res.noeuds_explores = e.noeuds;
  res.exact = !e.limite;
  res.erreur = cout(a_, b_, res.appariement, options_);
  if (res.erreur == CoutInvalide) res.erreur = e.meilleur_cout;
  return res;
}

}  // namespace nis
