// Chaine de traitement d'image : remplace `image.cpp` (classe `Image` basee sur
// IplImage/OpenCV).
//
// Le traitement historique etait :
//   lecture -> conversion 8U 1 canal -> binarisation -> supression du fond ->
//   squelettisation -> extraction des terminaisons et bifurcations -> graphe.
// Les fonctions ci-dessous font la meme chose, sans OpenCV, et corrige :
//  * `lectureImage()` testait `image->nChannels == 3` puis sortait sans
//    conversion : une image couleur etait refusee silencieusement ;
//  * `binarisation()` supposait 3 canaux et ecrivait dans un `IplImage`
//    jamais alloue correctement ;
//  * `suprime_back()` comparait des coordonnees entieres sans verifier les
//    bornes et modifiait la ligne pendant qu'elle la parcourait ;
//  * `amincissement1/2/3` tournaient en boucle `while(1)` sans garantie
//    d'arret, supprimaient des pixels deja lus dans la meme passe (dependance a
//    l'ordre de parcours) et sortaient sur des criteres de compteur
//    approximatifs ;
//  * `extraction_bifurcation()` ne regardait que la fenetre [80..200] de
//    l'image (zone codee en dur) et `extraction_sommet()` s'arretait a 5
//    minuties.
//
// Toutes les fonctions sont pures : elles renvoient un nouveau résultat, ne
// touchent pas l'entree, et ne depassent jamais du cadre de l'image.
#ifndef NIS_CORE_PIPELINE_IMAGE_H
#define NIS_CORE_PIPELINE_IMAGE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/Graphe.h"
#include "core/ImageNiveauxGris.h"

namespace nis {

struct OptionsPipeline {
  /// Seuil de binarisation : en dessous, encre ; au-dessus, fond.
  int seuil{128};
  /// Nombre maximal de passes d'amincissement (garde-fou d'arret).
  std::size_t max_passes{200};
  /// Deux minuties du meme type a moins de cette distance (pixels) ne forment
  /// qu'un seul sommet.  0 desactive la fusion.
  std::size_t distance_minimale{3};
  /// Nombre maximal de minuties conservees.  0 = aucune limite.
  std::size_t max_sommets{0};
};

struct EtapePipeline {
  std::size_t passes{0};
  bool converge{true};
};

namespace pipeline {

/// Binarisation : toute valeur strictement inferieure a `seuil` devient de
/// l'encre (0), le reste du fond (255).
ImageNiveauxGris binariser(const ImageNiveauxGris& src, int seuil);

/// Elimine le fond accroche aux bords gauche et droit de chaque ligne, comme
/// `suprime_back` : la zone blanche avant le premier trait et apres le dernier
/// devient du fond.  Utile quand la numerisation laisse une bande grise.
ImageNiveauxGris eliminerFond(const ImageNiveauxGris& src);

/// Amincissement de Zhang-Suen : la sortie a des traits d'un pixel de large.
/// Deux sous-passes par iteration, les suppressions sont marquees puis
/// appliquees, ce qui retire la dependance a l'ordre de parcours.
EtapePipeline amincir(ImageNiveauxGris* image, const OptionsPipeline& opt);

/// Minuties d'un squelette : extremite (un voisin) = terminaison, embranchement
/// (trois voisins hors bloc compact) = bifurcation.  L'angle en degrees est
/// calcule (il etait laisse a 0 en 2006) : direction du trait pour une
/// terminaison, moyenne des branches pour une bifurcation.
std::vector<Sommet> extraireMinuties(const ImageNiveauxGris& squelette,
                                     const OptionsPipeline& opt);

/// Graphe complet dont les poids sont les distances euclidiennes.
Graphe construireGraphe(const std::vector<Sommet>& sommets, const std::string& nom);

/// Chaine complete, etape par etape, pour l'affichage pas a pas de l'interface.
struct ResultatChaine {
  ImageNiveauxGris binarisee;
  ImageNiveauxGris sansFond;
  ImageNiveauxGris squelette;
  std::vector<Sommet> minuties;
  Graphe graphe;
  EtapePipeline amincissement;
};
ResultatChaine chaineComplete(const ImageNiveauxGris& src, const OptionsPipeline& opt);

}  // namespace pipeline
}  // namespace nis

#endif  // NIS_CORE_PIPELINE_IMAGE_H
