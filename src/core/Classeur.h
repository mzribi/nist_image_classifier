// Classification d'un ensemble de graphes : remplace `classificateur.cpp`.
//
// Le principe est conserve (k-moyennes sur graphes, centrois = graphes medians) ;
// les defauts corriges sont :
//  * `Classement()` comparait `erreur > min` au lieu de `erreur < min` : la
//    classe retenue etait systematiquement la plus eloignee ;
//  * les centrois initiaux etaient les `k` premiers graphes dans l'ordre du
//    repertoire, ce qui dependait du nom des fichiers et pouvait reunir deux
//    centrois identiques ;
//  * `compteur_classe` et `nbmax` n'etaient jamais remis a zero, `new[]` etait
//    libere avec `delete` (et non `delete[]`), et l'affectation se faisait sur
//    un tableau `C[]` de 100 elements en ecriture directe dans
//    `c:\...\classe N\` : chemin et taille codés en dur ;
//  * `klas.txt` etait ecrit puis relus par `intialiser()` sans verification
//    d'ouverture, et les centres etaient rechargés depuis
//    `c:\Users\sans dimensionnement\Desktop\nnouveau\a N\graphe_median.txt`.
//
// Ici : initialisation par point le plus eloigne (deterministe), affectation au
// centre le plus proche, recalcul des medians, arret sur stabilisation ou
// `max_iterations`.  Rien n'ecrit sur le disque : la classe `Classeur` est pure.
#ifndef NIS_CORE_CLASSEUR_H
#define NIS_CORE_CLASSEUR_H

#include <cstddef>
#include <string>
#include <vector>

#include "core/Graphe.h"
#include "core/GrapheMedian.h"

namespace nis {

struct Classe {
  Graphe median;
  std::vector<std::size_t> membres;  ///< indices des graphes dans l'ordre d'entree
  double cout{0.0};                  ///< somme des erreurs d'appariement des membres
};

struct OptionsClassification {
  std::size_t nb_classes{2};
  std::size_t max_iterations{10};
  OptionsMedian median;
};

struct ResultatClassement {
  std::vector<Classe> classes;
  /// `affectation[i]` : rang de la classe du graphe i, -1 si aucun centre.
  std::vector<int> affectation;
  std::size_t iterations{0};
  bool converge{false};
  bool annule{false};

  /// Nom de repertoire historique d'une classe : `classe 1`, `classe 2`, ...
  static std::string nomRepertoire(std::size_t rang) { return "classe " + std::to_string(rang + 1); }
};

class Classeur {
 public:
  explicit Classeur(OptionsClassification options = {});

  ResultatClassement classer(const std::vector<Graphe>& graphes) const;

  /// Rang du centre le plus proche de `g` (erreur minimale), -1 si `centres` est
  /// vide.  `erreur` reçoit le cout correspondant.
  static int classeLaPlusProche(const Graphe& g, const std::vector<Graphe>& centres,
                                const OptionsAppariement& options, double* erreur = nullptr);

 private:
  /// Indices des graphes choisis comme centrois initiaux.
  std::vector<std::size_t> initialiser(const std::vector<Graphe>& graphes, std::size_t k) const;

  OptionsClassification options_;
};

}  // namespace nis

#endif  // NIS_CORE_CLASSEUR_H
