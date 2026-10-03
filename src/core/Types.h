// Types communs au noyau de calcul.
//
// Remplace, en le nettoyant, le couple MFC `noeud` + la classe morte `Minutie`.
// Le noyau ne depend ni de Qt ni d'OpenCV : uniquement la standard library.
#ifndef NIS_CORE_TYPES_H
#define NIS_CORE_TYPES_H

#include <cmath>
#include <cstddef>

namespace nis {

/// Nature d'une minutie.  Les valeurs reprennent celles de l'ancien code
/// (`noeud::type` valait 0 pour une terminaison, 1 pour une bifurcation) afin
/// que les fichiers `graphe_median.txt` deja produits restent lisibles.
enum class TypeMinutie : int {
  Terminaison = 0,
  Bifurcation = 1,
};

/// Un sommet du graphe de minuties.
///
/// L'ancien code stockait `x` et `y` en `int` et un `angle` jamais utilise ;
/// les coordonnees passent en `double` (le graphe median produit des moyennes)
/// et l'angle est conserve car il sert a l'affichage et a une penalisation
/// optionnelle de l'appariement.
struct Sommet {
  TypeMinutie type{TypeMinutie::Terminaison};
  double x{0.0};
  double y{0.0};
  double angle{0.0};

  bool operator==(const Sommet& autre) const {
    return type == autre.type && x == autre.x && y == autre.y && angle == autre.angle;
  }
};

/// Distance euclidienne entre deux sommets : c'est la definition historique du
/// poids d'arete (`graphe::remplir_matrice`).
inline double distanceEuclidienne(const Sommet& a, const Sommet& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

/// Ecart d'etiquette entre deux minuties, dans l'esprit de la matrice `P`
/// historique (`abs(type1 - type2)`) mais rendu explicite.
inline double differenceType(TypeMinutie a, TypeMinutie b) {
  return static_cast<double>(static_cast<int>(a) - static_cast<int>(b));
}

}  // namespace nis

#endif  // NIS_CORE_TYPES_H
