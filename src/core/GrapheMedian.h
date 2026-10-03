// Graphe median d'un ensemble de graphes : remplace `graphe_median.cpp`.
//
// Algorithme de 2006 (et ses defauts) :
//  `graphe_median(tab, tab_median, nb_graph)` construisait les etiquettes du
//  median a partir des tableaux globaux `ES`, `EXT_ES`, `Ls`, `P` puis, pour
//  chaque valeur de l'alphabet, cherchait un « sommet apparie provisaire ».
//  Trois bugs majeurs :
//   1. les poids d'arete etaient recopies depuis `P[TAB[0][0]][TAB[1][0]]`, une
//      seule valeur reutilisee pour TOUTES les aretes (et TAB[0][0] pouvait
//      valoir -1 : lecture hors limites) ;
//   2. `if (EXT_ES[k] == NULL)` comparait un entier a NULL : les sommets
//      virtuels n'etaient jamais reconnus ;
//   3. aucune garantie d'arret, et le resultat dependait de l'ordre des
//      tableaux globaux.
//
// Algorithme remplace, toujours fidele a l'idee d'origine (un median dont les
// sommets resument les sommets apparies de chaque graphe) :
//  * initialisation : le graphe dont la taille est la plus proche de M, echantillonne
//    a M sommets ;
//  * a chaque iteration, chaque graphe est apparie au median courant
//    (`Appariement`, optimum garanti pour les petites tailles) ;
//  * le nouveau median prend, par creneau, l'etiquette majoritaire (projete sur
//    l'alphabet Ls) et les coordonnees moyennes des sommets qui lui sont
//    apparis ; les poids d'arete sont la moyenne des poids reellement observes ;
//  * on s'arrete quand les appariements se stabilisent, quand le cout total
//    remonte, ou a `max_iterations`.  Le meilleur median rencontre est renvoye.
//
// Le couplage etait precedent : `EnsembleGraphes::preparer()` doit avoir ete
// appele (il fournit M et l'alphabet).
#ifndef NIS_CORE_GRAPHE_MEDIAN_H
#define NIS_CORE_GRAPHE_MEDIAN_H

#include <cstddef>
#include <functional>
#include <vector>

#include "core/Appariement.h"
#include "core/EnsembleGraphes.h"
#include "core/Graphe.h"

namespace nis {

struct OptionsMedian {
  std::size_t max_iterations{10};
  OptionsAppariement appariement;
  /// Appelee avant chaque iteration ; renvoyer faux pour interrompre le calcul.
  std::function<bool()> annulation;
};

struct ResultatMedian {
  Graphe graphe;
  /// `appariements[g][i]` : creneau du median du sommet i du graphe g, ou -1.
  std::vector<std::vector<int>> appariements;
  double cout_total{0.0};
  std::size_t iterations{0};
  bool converge{false};
  bool annule{false};
};

class GrapheMedian {
 public:
  explicit GrapheMedian(OptionsMedian options = {});

  /// L'ensemble doit etre prepare (`EnsembleGraphes::preparer`).
  ResultatMedian calculer(const EnsembleGraphes& ensemble) const;

  /// Cout total d'un median vis-a-vis d'un ensemble : somme des meilleurs couts
  /// d'appariement de chaque graphe au median.
  static double coutTotal(const std::vector<Graphe>& graphes, const Graphe& median,
                          const OptionsAppariement& options);

  const OptionsMedian& options() const { return options_; }

 private:
  Graphe construireMedian(const std::vector<Graphe>& graphes,
                          const std::vector<std::vector<int>>& appariements,
                          const Graphe& precedent, std::size_t taille,
                          const std::vector<int>& alphabet) const;
  Graphe graine(const std::vector<Graphe>& graphes, std::size_t taille) const;

  OptionsMedian options_;
};

}  // namespace nis

#endif  // NIS_CORE_GRAPHE_MEDIAN_H
