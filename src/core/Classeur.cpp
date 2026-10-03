#include "core/Classeur.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "core/Appariement.h"

namespace nis {
namespace {

double distanceGraphes(const Graphe& a, const Graphe& b, const OptionsAppariement& opt) {
  return Appariement(a, b, opt).calculer().erreur;
}

bool memesSommets(const Graphe& a, const Graphe& b) { return a.sommets() == b.sommets(); }

}  // namespace

Classeur::Classeur(OptionsClassification options) : options_(options) {}

int Classeur::classeLaPlusProche(const Graphe& g, const std::vector<Graphe>& centres,
                                 const OptionsAppariement& options, double* erreur) {
  if (centres.empty()) {
    if (erreur) *erreur = 0.0;
    return -1;
  }
  int meilleur_rang = -1;
  double meilleure = std::numeric_limits<double>::infinity();
  for (std::size_t c = 0; c < centres.size(); ++c) {
    const double e = distanceGraphes(g, centres[c], options);
    if (e < meilleure - 1e-12) {
      meilleure = e;
      meilleur_rang = static_cast<int>(c);
    }
  }
  if (erreur) *erreur = meilleure;
  return meilleur_rang;
}

std::vector<std::size_t> Classeur::initialiser(const std::vector<Graphe>& graphes,
                                               std::size_t k) const {
  std::vector<std::size_t> centres;
  const std::size_t n = graphes.size();
  if (n == 0 || k == 0) return centres;

  // Premier centre : graphe dont la taille est la plus proche de la moyenne.
  double moyenne = 0.0;
  for (const Graphe& g : graphes) moyenne += static_cast<double>(g.nbSommets());
  moyenne /= static_cast<double>(n);

  std::size_t premier = 0;
  double ecart = std::fabs(static_cast<double>(graphes[0].nbSommets()) - moyenne);
  for (std::size_t i = 1; i < n; ++i) {
    const double e = std::fabs(static_cast<double>(graphes[i].nbSommets()) - moyenne);
    if (e < ecart - 1e-12) {
      ecart = e;
      premier = i;
    }
  }
  centres.push_back(premier);

  // Centres suivants : point le plus eloigne du plus proche centre deja choisi
  // (k-medoids « farthest point », deterministic contrairement a l'ancien code).
  std::vector<double> distance_min(n, std::numeric_limits<double>::infinity());
  while (centres.size() < k && centres.size() < n) {
    const std::size_t recent = centres.back();
    for (std::size_t i = 0; i < n; ++i) {
      distance_min[i] = std::min(distance_min[i],
                                 distanceGraphes(graphes[i], graphes[recent], options_.median.appariement));
    }
    std::size_t suivant = 0;
    double plus_grande = -1.0;
    for (std::size_t i = 0; i < n; ++i) {
      if (std::find(centres.begin(), centres.end(), i) != centres.end()) continue;
      if (distance_min[i] > plus_grande + 1e-12) {
        plus_grande = distance_min[i];
        suivant = i;
      }
    }
    centres.push_back(suivant);
  }
  return centres;
}

ResultatClassement Classeur::classer(const std::vector<Graphe>& graphes) const {
  ResultatClassement res;
  const std::size_t n = graphes.size();
  if (n == 0) {
    res.converge = true;
    return res;
  }
  const std::size_t k = std::max<std::size_t>(1, std::min(options_.nb_classes, n));

  std::vector<Graphe> centres;
  centres.reserve(k);
  for (const std::size_t idx : initialiser(graphes, k)) centres.push_back(graphes[idx]);

  const OptionsAppariement& opt = options_.median.appariement;
  std::vector<int> affectation_prec(n, -1);
  std::vector<int> affectation(n, -1);
  std::vector<Classe> classes(k);

  const std::size_t max_iterations = std::max<std::size_t>(1, options_.max_iterations);
  for (std::size_t iteration = 1; iteration <= max_iterations; ++iteration) {
    if (options_.median.annulation && !options_.median.annulation()) {
      res.annule = true;
      break;
    }
    res.iterations = iteration;

    // --- Affectation au centre le plus proche (et non le plus loin : bug 2006).
    for (std::size_t i = 0; i < n; ++i) {
      affectation[i] = classeLaPlusProche(graphes[i], centres, opt, nullptr);
    }

    // --- Recalcul d'un median par classe.
    classes.assign(k, Classe());
    for (std::size_t c = 0; c < k; ++c) {
      classes[c].membres.clear();
      for (std::size_t i = 0; i < n; ++i) {
        if (affectation[i] == static_cast<int>(c)) classes[c].membres.push_back(i);
      }
      if (classes[c].membres.empty()) {
        classes[c].median = centres[c];  // classe vide : centre conserve
        continue;
      }
      EnsembleGraphes ensemble;
      for (const std::size_t m : classes[c].membres) ensemble.ajouter(graphes[m]);
      std::string erreur;
      if (!ensemble.preparer(&erreur)) {
        classes[c].median = centres[c];
        continue;
      }
      const ResultatMedian r = GrapheMedian(options_.median).calculer(ensemble);
      classes[c].median = r.graphe;
      classes[c].cout = r.cout_total;
    }

    const bool stable = (affectation == affectation_prec);
    std::vector<Graphe> nouveaux;
    nouveaux.reserve(k);
    for (const Classe& c : classes) nouveaux.push_back(c.median);

    if (stable) {
      centres = nouveaux;
      res.converge = true;
      break;
    }
    affectation_prec = affectation;
    centres = std::move(nouveaux);
  }

  res.classes = classes;
  res.affectation = affectation;
  return res;
}

}  // namespace nis
