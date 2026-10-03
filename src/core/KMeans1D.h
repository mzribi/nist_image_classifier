// K-moyennes 1D : remplace `k_means.cpp` (fonction libre + entiers).
//
// L'algorithme sert uniquement a reduire l'alphabet des etiquettes de minuties
// (valeurs 0/1) avant le calcul du graphe median.  Trois bugs de 2006 sont
// corriges ici :
//  * les centrois etaient la MEDIANE des affectes (`C[0]+C[k/2]+C[k-1])/3`) au
//    lieu de la moyenne, et cette formule n'avait aucun sens quand k < 2 ;
//  * la moyenne etait faite en arithmetique entiere (`som / k`), ce qui ecrasait
//    les centrois vers 0 ;
//  * la boucle `do { } while(!fini)` tournait sans borne : si deux centrois
//    restaient a egalite distance, le programme bouclait indefiniment.
#ifndef NIS_CORE_KMEANS1D_H
#define NIS_CORE_KMEANS1D_H

#include <cstddef>
#include <vector>

namespace nis {

struct ResultatKMeans {
  std::vector<int> etiquettes;  ///< une etiquette par valeur d'entree
  std::vector<int> centrois;    ///< taille = k effectif
  std::size_t iterations{0};
  bool converge{false};         ///< faux si max_iterations a ete atteint
  std::size_t kDemande{0};
  std::size_t kEffectif{0};
};

/// K-moyennes deterministe sur des valeurs entieres.
class KMeans1D {
 public:
  KMeans1D() = default;
  explicit KMeans1D(std::vector<int> valeurs);

  /// k est ramene au nombre de valeurs distinctes (k ne peut jamais depasser
  /// le nombre de classes reellement separables).  Retourne un resultat vide
  /// si `valeurs_` est vide.
  ResultatKMeans lancer(std::size_t k, std::size_t max_iterations = 100) const;

  const std::vector<int>& valeurs() const { return valeurs_; }

 private:
  std::vector<int> valeurs_;
};

}  // namespace nis

#endif  // NIS_CORE_KMEANS1D_H
