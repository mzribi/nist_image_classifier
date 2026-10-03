#include "core/GrapheMedian.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nis {
namespace {

/// Projette une etiquette moyenne sur l'alphabet Ls (l'etiquette du median doit
/// rester une etiquette de minutie valide, pas une moyenne fractionnaire).
int projeterSurAlphabet(double moyenne, const std::vector<int>& alphabet) {
  int meilleur = alphabet.empty() ? 0 : alphabet.front();
  double ecart = std::fabs(moyenne - meilleur);
  for (const int v : alphabet) {
    const double d = std::fabs(moyenne - v);
    if (d < ecart) {
      ecart = d;
      meilleur = v;
    }
  }
  return std::max(0, std::min(1, meilleur));
}

/// Echantillonne regulierement `taille` sommets dans `g` (indices arrondis).
Graphe echantillonner(const Graphe& g, std::size_t taille) {
  const std::size_t n = g.nbSommets();
  std::vector<Sommet> sommets;
  sommets.reserve(taille);
  if (n > 0) {
    for (std::size_t k = 0; k < taille; ++k) {
      std::size_t idx = 0;
      if (taille > 1 && n > 1) {
        idx = static_cast<std::size_t>(std::lround(static_cast<double>(k) *
                                                   static_cast<double>(n - 1) /
                                                   static_cast<double>(taille - 1)));
      }
      sommets.push_back(g.sommet(std::min(idx, n - 1)));
    }
  }
  return Graphe(std::move(sommets));
}

}  // namespace

GrapheMedian::GrapheMedian(OptionsMedian options) : options_(options) {}

Graphe GrapheMedian::graine(const std::vector<Graphe>& graphes, std::size_t taille) const {
  std::size_t meilleur = 0;
  std::size_t ecart = std::numeric_limits<std::size_t>::max();
  for (std::size_t g = 0; g < graphes.size(); ++g) {
    if (graphes[g].nbSommets() == 0) continue;
    const std::size_t e = (graphes[g].nbSommets() > taille)
                              ? graphes[g].nbSommets() - taille
                              : taille - graphes[g].nbSommets();
    if (e < ecart) {
      ecart = e;
      meilleur = g;
    }
  }
  return echantillonner(graphes[meilleur], taille);
}

Graphe GrapheMedian::construireMedian(const std::vector<Graphe>& graphes,
                                      const std::vector<std::vector<int>>& appariements,
                                      const Graphe& precedent, std::size_t taille,
                                      const std::vector<int>& alphabet) const {
  std::vector<Sommet> sommets(taille);

  for (std::size_t s = 0; s < taille; ++s) {
    double somme_x = 0.0, somme_y = 0.0, somme_angle = 0.0, somme_type = 0.0;
    std::size_t compteur = 0;
    for (std::size_t g = 0; g < graphes.size(); ++g) {
      if (g >= appariements.size()) continue;
      const std::vector<int>& ap = appariements[g];
      for (std::size_t i = 0; i < graphes[g].nbSommets() && i < ap.size(); ++i) {
        if (ap[i] != static_cast<int>(s)) continue;
        const Sommet& som = graphes[g].sommet(i);
        somme_x += som.x;
        somme_y += som.y;
        somme_angle += som.angle;
        somme_type += static_cast<int>(som.type);
        ++compteur;
      }
    }
    if (compteur == 0) {
      // Creneau sans sommet apparie : on conserve la valeur precedente (le
      // median garde M sommets, comme les sommets virtuels de 2006).
      sommets[s] = precedent.sommet(s);
      continue;
    }
    const double n = static_cast<double>(compteur);
    sommets[s].type = (projeterSurAlphabet(somme_type / n, alphabet) == 0)
                          ? TypeMinutie::Terminaison
                          : TypeMinutie::Bifurcation;
    sommets[s].x = somme_x / n;
    sommets[s].y = somme_y / n;
    sommets[s].angle = somme_angle / n;
  }

  Graphe median(std::move(sommets));

  // --- Poids d'arete : moyenne des poids reellement observes, arete par arete.
  // C'est le point precis ou l'ancien code recopiait une seule valeur pour tout.
  Graphe::Matrice matrice(taille, std::vector<double>(taille, 0.0));
  for (std::size_t s = 0; s < taille; ++s) {
    matrice[s][s] = 0.0;
    for (std::size_t u = s + 1; u < taille; ++u) {
      double somme = 0.0;
      std::size_t compteur = 0;
      for (std::size_t g = 0; g < graphes.size(); ++g) {
        if (g >= appariements.size() || appariements[g].size() != graphes[g].nbSommets()) continue;
        const std::vector<int>& ap = appariements[g];
        // un seul sommet par creneau (l'appariement est une injection)
        int is = -1, iu = -1;
        for (std::size_t i = 0; i < ap.size(); ++i) {
          if (ap[i] == static_cast<int>(s)) is = static_cast<int>(i);
          else if (ap[i] == static_cast<int>(u)) iu = static_cast<int>(i);
        }
        if (is >= 0 && iu >= 0) {
          somme += graphes[g].poids(static_cast<std::size_t>(is), static_cast<std::size_t>(iu));
          ++compteur;
        }
      }
      const double poids = (compteur > 0)
                               ? somme / static_cast<double>(compteur)
                               : distanceEuclidienne(median.sommet(s), median.sommet(u));
      matrice[s][u] = poids;
      matrice[u][s] = poids;
    }
  }
  median.setMatrice(std::move(matrice));
  median.setNom("graphe_median");
  return median;
}

double GrapheMedian::coutTotal(const std::vector<Graphe>& graphes, const Graphe& median,
                               const OptionsAppariement& options) {
  double total = 0.0;
  for (const Graphe& g : graphes) {
    total += Appariement(g, median, options).calculer().erreur;
  }
  return total;
}

ResultatMedian GrapheMedian::calculer(const EnsembleGraphes& ensemble) const {
  ResultatMedian res;
  const std::vector<Graphe>& graphes = ensemble.graphes();
  const std::size_t taille = ensemble.tailleMedian();
  if (graphes.empty() || taille == 0) {
    res.graphe = Graphe({}, "graphe_median");
    res.converge = true;
    return res;
  }

  Graphe median = graine(graphes, taille);
  median.setNom("graphe_median");

  std::vector<std::vector<int>> appariements_prec(graphes.size());
  std::vector<std::vector<int>> appariements_meilleurs(graphes.size());
  Graphe median_meilleur = median;
  double cout_meilleur = std::numeric_limits<double>::infinity();

  const std::size_t max_iterations = std::max<std::size_t>(1, options_.max_iterations);
  for (std::size_t iteration = 1; iteration <= max_iterations; ++iteration) {
    if (options_.annulation && !options_.annulation()) {
      res.annule = true;
      break;
    }
    res.iterations = iteration;

    std::vector<std::vector<int>> appariements(graphes.size());
    double total = 0.0;
    for (std::size_t g = 0; g < graphes.size(); ++g) {
      const ResultatAppariement r = Appariement(graphes[g], median, options_.appariement).calculer();
      appariements[g] = r.appariement;
      total += r.erreur;
    }

    if (total < cout_meilleur - 1e-9) {
      cout_meilleur = total;
      median_meilleur = median;
      appariements_meilleurs = appariements;
    }

    if (appariements == appariements_prec) {
      res.converge = true;
      break;
    }
    appariements_prec = appariements;
    median = construireMedian(graphes, appariements, median, taille, ensemble.alphabet());
  }

  if (!std::isfinite(cout_meilleur)) {
    // max_iterations == 0 ou annulation immediate : on retombe sur la graine.
    cout_meilleur = coutTotal(graphes, median, options_.appariement);
    median_meilleur = median;
  }

  res.graphe = median_meilleur;
  res.appariements = appariements_meilleurs;
  res.cout_total = cout_meilleur;
  return res;
}

}  // namespace nis

