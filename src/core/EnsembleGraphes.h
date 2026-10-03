// Ensemble de graphes : remplace `ens_graph` (tableaux fixes, `new`/`delete`,
// etats jamais reinitialises).
//
// L'ensemble porte les grandeurs derivees necessaires au calcul du graphe
// median :
//  * `taille_median()` (M historique) : nombre de sommets du graphe median,
//    moyenne arrondie des tailles des graphes ;
//  * `etiquettes()` (ES) : toutes les etiquettes de minuties concatenees ;
//  * `etiquettesEtendues()` (EXT_ES) : ES completee par des sommets virtuels
//    (-1) pour que chaque graphe occupe une fenetre de max(M, taille) colonnes ;
//  * `alphabet()` (Ls) : alphabet reduit des etiquettes, obtenu par k-moyennes.
//
// L'ancien code exprimait ces notions via `graphe_associe()`, qui parcourait
// `Lg` a chaque appel sans jamais remettre `som` a zero (deuxieme appel => faux).
// Les acces sont ici directs et sans etat cache.
#ifndef NIS_CORE_ENSEMBLE_GRAPHES_H
#define NIS_CORE_ENSEMBLE_GRAPHES_H

#include <cstddef>
#include <string>
#include <vector>

#include "core/Graphe.h"

namespace nis {

class EnsembleGraphes {
 public:
  /// Valeur d'etiquette marquant une colonne hors du graphe (sommets virtuels).
  static constexpr int SommetVirtuel = -1;

  void ajouter(Graphe graphe);
  void vider();

  std::size_t nbGraphes() const { return graphes_.size(); }
  const std::vector<Graphe>& graphes() const { return graphes_; }
  const Graphe& graphe(std::size_t indice) const;
  std::size_t nbSommetsTotal() const;

  /// Recalcule toutes les grandeurs derivees.  Faux + message si l'ensemble est
  /// vide ou si aucun graphe n'a de sommet.
  bool preparer(std::string* erreur = nullptr);
  bool pret() const { return pret_; }

  std::size_t tailleMedian() const { return taille_median_; }
  std::size_t nbColonnes() const { return nb_colonnes_; }
  const std::vector<int>& etiquettes() const { return etiquettes_; }
  const std::vector<int>& etiquettesEtendues() const { return etiquettes_etendues_; }
  const std::vector<int>& alphabet() const { return alphabet_; }

  /// Fenetre de colonnes occupee par le graphe `g`.
  std::size_t colonneInitiale(std::size_t g) const;
  std::size_t largeurFenetre(std::size_t g) const;
  std::size_t colonneGlobale(std::size_t g, std::size_t locale) const;

  /// Decoupe une colonne globale en (graphe, index local).  Faux si hors
  /// intervalle.  Un index local superieur ou egal a la taille du graphe designe
  /// un sommet virtuel.
  bool colonneLocale(std::size_t globale, std::size_t* graphe, std::size_t* locale) const;
  /// Etiquette d'une colonne globale, `SommetVirtuel` si colonne virtuelle.
  int etiquetteColonne(std::size_t globale) const;
  /// Vrai si la colonne globale correspond a un vrai sommet du graphe.
  bool colonneReelle(std::size_t globale) const;

 private:
  std::vector<Graphe> graphes_;
  bool pret_{false};
  std::size_t taille_median_{0};
  std::size_t nb_colonnes_{0};
  std::vector<int> etiquettes_;
  std::vector<int> etiquettes_etendues_;
  std::vector<int> alphabet_;
  std::vector<std::size_t> debut_;
  std::vector<std::size_t> largeur_;
};

}  // namespace nis

#endif  // NIS_CORE_ENSEMBLE_GRAPHES_H
