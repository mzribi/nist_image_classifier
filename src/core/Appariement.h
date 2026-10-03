// Appariement de deux graphes : remplace `appariement.cpp`.
//
// Le role (chercher la mise en correspondance des sommets qui minimise un cout)
// est conserve.  Defauts corriges :
//  * `appariement()` cherchait le minimum puis `appariement1()` le MAXIMUM
//    (`if (cout1 > cout2) f = cout1;`) : l'erreur transmise a la classification
//    etait donc la pire des solutions rencontrees ;
//  * `Tirage_Aleatoire()` n'etait jamais appele : `Tirage_A` restait a zero, le
//    tirage renvoyait toujours le meme graphe, et la variable `alea`, jamais
//    initialisee, servait dans un test de sortie de boucle ;
//  * les tableaux `p/q/nb_p/nb_q/fond`, alloues par `new`, etaient liberes par
//    `delete[] tab` au lieu de `delete[] tab[i]` : fuites et memoire liberee
//    invalide ;
//  * `remplissage_mat` ecrivait `T[i][j] = 0` pour un appariement virtuel, ce
//    qui creait une arete de poids nul (donc ideale) au lieu d'une penalite ;
//  * la recherche s'arretait apres `nb_tentative` essais sans garantie, et
//    `erreur` pouvait rester non initialisee.
//
// Nouvelle methode : exploration par separation et bornes (branche et limite)
// sur les injections partielles.  Tous les couts etant positifs, le cout partiel
// est une minoration du cout total : la borne est admissible et l'optimum est
// garanti tant que le nombre de noeuds explores reste sous `max_noeuds`
// (`ResultatAppariement::exact` rend compte de la limite atteinte).
#ifndef NIS_CORE_APPARIEMENT_H
#define NIS_CORE_APPARIEMENT_H

#include <cstddef>
#include <vector>

#include "core/Graphe.h"

namespace nis {

struct OptionsAppariement {
  /// Cout d'un desaccord d'etiquette, multiplie par |type(A) - type(B)| (0 ou 1).
  /// A 100, une inversion terminaison/bifurcation coute autant que 100 pixels
  /// d'ecart de distance : les etiquettes restent dominantes sans saturer.
  double penalite_etiquette{100.0};
  /// Cout d'une arete qui disparait quand un sommet est apparie a rien.
  /// L'ancien code mettait 0, ce qui rendait l'appariement virtuel gratuit.
  double penalite_arete{50.0};
  /// Cout d'un sommet de A non apparie.
  double penalite_sommet{100.0};
  /// Nombre de sommets en dessous duquel la recherche est exhaustive.
  std::size_t seuil_exhaustif{8};
  /// Garde-fou de temps : nombre maximal de noeuds explores.
  std::size_t max_noeuds{400000};
};

struct ResultatAppariement {
  /// `appariement[i]` = indice du sommet de B, ou -1 si le sommet i de A n'est
  /// apparie a rien (sommet virtuel).
  std::vector<int> appariement;
  double erreur{0.0};
  std::size_t noeuds_explores{0};
  bool exact{false};  ///< faux si la limite `max_noeuds` a coupe la recherche
};

class Appariement {
 public:
  Appariement(const Graphe& a, const Graphe& b, OptionsAppariement options = {});

  ResultatAppariement calculer() const;

  /// Cout d'un appariement donne.  Renvoie l'infini si l'appariement est
  /// invalide (taille, valeur hors intervalle, sommet de B utilise deux fois).
  static double cout(const Graphe& a, const Graphe& b, const std::vector<int>& appariement,
                     const OptionsAppariement& options);

  /// Verifie la validite d'un appariement (meme regles que `cout`).
  static bool estValide(const Graphe& a, const Graphe& b, const std::vector<int>& appariement);

 private:
  const Graphe& a_;
  const Graphe& b_;
  OptionsAppariement options_;
};

}  // namespace nis

#endif  // NIS_CORE_APPARIEMENT_H
