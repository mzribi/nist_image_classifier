#include "core/KMeans1D.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

namespace nis {
namespace {

/// Arrondi bancaire evite : on arrondit a l'entier le plus proche, a 0.5 vers
/// le haut, comme le faisait implicitement le code historique.
int arrondir(double v) { return static_cast<int>(std::lround(v)); }

}  // namespace

KMeans1D::KMeans1D(std::vector<int> valeurs) : valeurs_(std::move(valeurs)) {}

ResultatKMeans KMeans1D::lancer(std::size_t k, std::size_t max_iterations) const {
  ResultatKMeans res;
  res.kDemande = k;
  res.etiquettes.assign(valeurs_.size(), 0);
  if (valeurs_.empty() || k == 0) return res;

  // Alphabet distinct trie : sert a une initialisation deterministe (l'ancien
  // code dependait de l'ordre d'apparition dans ES).
  const std::set<int> distinct(valeurs_.begin(), valeurs_.end());
  std::vector<int> valeurs_distinctes(distinct.begin(), distinct.end());

  const std::size_t k_eff = std::min(k, valeurs_distinctes.size());
  res.kEffectif = k_eff;

  std::vector<double> centrois(k_eff, 0.0);
  if (k_eff == 1) {
    centrois[0] = valeurs_distinctes[valeurs_distinctes.size() / 2];
  } else {
    for (std::size_t i = 0; i < k_eff; ++i) {
      const std::size_t idx = (i * (valeurs_distinctes.size() - 1)) / (k_eff - 1);
      centrois[i] = static_cast<double>(valeurs_distinctes[idx]);
    }
  }

  std::vector<std::size_t> comptes(k_eff, 0);
  for (std::size_t iteration = 1; iteration <= max_iterations; ++iteration) {
    res.iterations = iteration;

    // --- Affectation : plus proche centrois, egalite => plus petit indice.
    bool changement = false;
    for (std::size_t i = 0; i < valeurs_.size(); ++i) {
      std::size_t meilleur = 0;
      double distance_meilleure = std::fabs(valeurs_[i] - centrois[0]);
      for (std::size_t c = 1; c < k_eff; ++c) {
        const double d = std::fabs(valeurs_[i] - centrois[c]);
        if (d < distance_meilleure) {
          distance_meilleure = d;
          meilleur = c;
        }
      }
      if (static_cast<std::size_t>(res.etiquettes[i]) != meilleur) {
        res.etiquettes[i] = static_cast<int>(meilleur);
        changement = true;
      }
    }

    // --- Recalcul des centrois : vraie moyenne arithmetique.
    std::vector<double> sommes(k_eff, 0.0);
    std::fill(comptes.begin(), comptes.end(), 0);
    for (std::size_t i = 0; i < valeurs_.size(); ++i) {
      const std::size_t c = static_cast<std::size_t>(res.etiquettes[i]);
      sommes[c] += valeurs_[i];
      ++comptes[c];
    }
    for (std::size_t c = 0; c < k_eff; ++c) {
      if (comptes[c] > 0) sommes[c] /= static_cast<double>(comptes[c]);
      else sommes[c] = centrois[c];  // centrois vide conserve
    }

    // Les centrois restent entiers : c'est l'hypothese de l'alphabet Ls du
    // graphe median.  L'arondi peut provoquer un flottement entre deux solutions
    // equitantes, d'ou la borne sur max_iterations.
    std::vector<int> arrondis(k_eff, 0);
    for (std::size_t c = 0; c < k_eff; ++c) arrondis[c] = arrondir(sommes[c]);

    bool centrois_stables = true;
    for (std::size_t c = 0; c < k_eff; ++c) {
      if (static_cast<double>(arrondis[c]) != centrois[c]) centrois_stables = false;
      centrois[c] = static_cast<double>(arrondis[c]);
    }
    res.centrois = arrondis;
    if (centrois_stables && !changement) {
      res.converge = true;
      break;
    }
  }
  return res;
}

}  // namespace nis
