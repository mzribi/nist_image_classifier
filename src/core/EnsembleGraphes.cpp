#include "core/EnsembleGraphes.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "core/KMeans1D.h"

namespace nis {

void EnsembleGraphes::ajouter(Graphe graphe) {
  graphes_.push_back(std::move(graphe));
  pret_ = false;
}

void EnsembleGraphes::vider() {
  graphes_.clear();
  etiquettes_.clear();
  etiquettes_etendues_.clear();
  alphabet_.clear();
  debut_.clear();
  largeur_.clear();
  taille_median_ = 0;
  nb_colonnes_ = 0;
  pret_ = false;
}

const Graphe& EnsembleGraphes::graphe(std::size_t indice) const {
  if (indice >= graphes_.size())
    throw std::out_of_range("EnsembleGraphes::graphe : indice hors limites");
  return graphes_[indice];
}

std::size_t EnsembleGraphes::nbSommetsTotal() const {
  std::size_t total = 0;
  for (const Graphe& g : graphes_) total += g.nbSommets();
  return total;
}

bool EnsembleGraphes::preparer(std::string* erreur) {
  pret_ = false;
  if (erreur) erreur->clear();

  if (graphes_.empty()) {
    if (erreur) *erreur = "aucun graphe dans l'ensemble";
    return false;
  }
  if (nbSommetsTotal() == 0) {
    if (erreur) *erreur = "tous les graphes sont vides : aucune minutie extraite";
    return false;
  }

  // --- M : moyenne arrondie des nombres de sommets (comportement historique).
  const std::size_t total = nbSommetsTotal();
  taille_median_ = std::max<std::size_t>(
      1, static_cast<std::size_t>(std::lround(
             static_cast<double>(total) / static_cast<double>(graphes_.size()))));

  // --- Fenetres de colonnes.
  debut_.assign(graphes_.size(), 0);
  largeur_.assign(graphes_.size(), 0);
  nb_colonnes_ = 0;
  etiquettes_.clear();
  etiquettes_etendues_.clear();
  for (std::size_t g = 0; g < graphes_.size(); ++g) {
    const std::size_t nb = graphes_[g].nbSommets();
    const std::size_t largeur = std::max(taille_median_, nb);
    debut_[g] = nb_colonnes_;
    largeur_[g] = largeur;
    nb_colonnes_ += largeur;
    for (std::size_t c = 0; c < largeur; ++c) {
      if (c < nb) {
        etiquettes_.push_back(static_cast<int>(graphes_[g].sommet(c).type));
        etiquettes_etendues_.push_back(static_cast<int>(graphes_[g].sommet(c).type));
      } else {
        etiquettes_etendues_.push_back(SommetVirtuel);
      }
    }
  }

  // --- Alphabet reduit Ls.  L'ancien code imposait 2*M centrois ; on demande
  // pareil mais KMeans1D ramene k au nombre de valeurs distinctes, ce qui
  // evite des centrois vides et des indices invalides.
  const std::vector<int> valeurs(etiquettes_.begin(), etiquettes_.end());
  const ResultatKMeans km = KMeans1D(valeurs).lancer(2 * taille_median_);
  alphabet_ = km.centrois;
  if (alphabet_.empty()) alphabet_.push_back(valeurs.front());

  pret_ = true;
  return true;
}

std::size_t EnsembleGraphes::colonneInitiale(std::size_t g) const {
  if (g >= debut_.size()) throw std::out_of_range("EnsembleGraphes::colonneInitiale");
  return debut_[g];
}

std::size_t EnsembleGraphes::largeurFenetre(std::size_t g) const {
  if (g >= largeur_.size()) throw std::out_of_range("EnsembleGraphes::largeurFenetre");
  return largeur_[g];
}

std::size_t EnsembleGraphes::colonneGlobale(std::size_t g, std::size_t locale) const {
  if (g >= debut_.size() || locale >= largeur_[g])
    throw std::out_of_range("EnsembleGraphes::colonneGlobale");
  return debut_[g] + locale;
}

bool EnsembleGraphes::colonneLocale(std::size_t globale, std::size_t* graphe,
                                    std::size_t* locale) const {
  if (graphe == nullptr || locale == nullptr) return false;
  for (std::size_t g = 0; g < debut_.size(); ++g) {
    if (globale >= debut_[g] && globale < debut_[g] + largeur_[g]) {
      *graphe = g;
      *locale = globale - debut_[g];
      return true;
    }
  }
  return false;
}

int EnsembleGraphes::etiquetteColonne(std::size_t globale) const {
  if (globale >= etiquettes_etendues_.size()) return SommetVirtuel;
  return etiquettes_etendues_[globale];
}

bool EnsembleGraphes::colonneReelle(std::size_t globale) const {
  std::size_t g = 0, locale = 0;
  if (!colonneLocale(globale, &g, &locale)) return false;
  return locale < graphes_[g].nbSommets();
}

}  // namespace nis
