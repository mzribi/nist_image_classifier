// Construction de la base de donnees : remplace la partie « classeur » de
// `ClassificationImage.cpp` (le code MFC qui melait UI, disques et algo).
//
// Le role est conserve : a partir d'un lot d'images, produire les graphes de
// minuties, les regrouper en N classes, puis ecrire dans
// `<dossier>/classe N/graphe_median.txt` (format legacy) et copier les images
// dans le dossier de leur classe.
//
// Ce qui change :
//  * le nom du repertoire de sortie est une donnee d'entree (plus de
//    `c:\Users\...\a 1` en dur) ;
//  * le decodage des images est injecte (`ChargeurImage`) : ce fichier ne depend
//    pas de Qt et se teste avec des images synthetiques ;
//  * la progression et l'annulation sont des rappels, pas de boites de dialogue
//    (`Sleep(200)` + `MessageBox` de 2006) ;
//  * chaque fichier en erreur est signale sans interrompre le lot ;
//  * les centres sont rendus dans la reponse : l'appelant decide de les
//    memoriser (l'ancien code passait l'etat par le fichier `c:\12.txt`).
#ifndef NIS_CORE_CONSTRUCTEUR_BASE_H
#define NIS_CORE_CONSTRUCTEUR_BASE_H

#include <functional>
#include <string>
#include <vector>

#include "core/Classeur.h"
#include "core/PipelineImage.h"

namespace nis {

/// Charge une image depuis un chemin.  Faux + message si impossible.
using ChargeurImage = std::function<bool(const std::string& chemin, ImageNiveauxGris* sortie,
                                         std::string* erreur)>;

/// Rapport d'avancement : (fraction entre 0 et 1, libelle).  Renvoyer faux pour
/// demander l'arret du traitement.
using Progression = std::function<bool(double, const std::string&)>;

struct OptionsBase {
  OptionsPipeline pipeline;
  OptionsClassification classification;
  /// Copier les images dans le dossier de leur classe.
  bool copier_images{true};
};

struct ResultatImage {
  std::string fichier;
  Graphe graphe;
  int classe{-1};            ///< rang de classe, -1 si non classe
  double erreur{0.0};        ///< cout d'appariement au centre de sa classe
  std::string echec;         ///< message si le fichier n'a pas pu etre traite
};

struct ResultatBase {
  std::vector<ResultatImage> images;
  std::vector<Classe> classes;
  std::vector<std::string> avertissements;
  std::string dossier_sortie;
  std::size_t graphes_ecrits{0};
  bool annule{false};

  /// Centres (graphes medians) dans l'ordre des classes : a conserver pour
  /// classifier une image isolee ensuite.
  std::vector<Graphe> centres() const;
};

class ConstructeurBase {
 public:
  ConstructeurBase(OptionsBase options, ChargeurImage chargeur);

  ResultatBase construire(const std::vector<std::string>& fichiers,
                          const std::string& dossier_sortie,
                          const Progression& progression = {}) const;

  /// Erreur de chaque centre vis-a-vis d'une image : `erreurs[c]` est le cout
  /// d'appariement du graphe au centre c.  Sert a l'affichage « erreur par
  /// classe » de l'interface de traitement d'image.
  static std::vector<double> erreursParClasse(const Graphe& graphe,
                                              const std::vector<Graphe>& centres,
                                              const OptionsAppariement& options);

 private:
  OptionsBase options_;
  ChargeurImage chargeur_;
};

}  // namespace nis

#endif  // NIS_CORE_CONSTRUCTEUR_BASE_H
