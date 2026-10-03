// Graphe de minuties : sommets etiquetes + matrice de poids d'aretes.
//
// Remplace `noeud.h`/`graphe.h` du projet MFC.  Corrections apportees :
//  * plus aucun tableau de taille fixe (`Max 20`, `matrice[20][20]`) ;
//  * les sommets sont possedes par valeur (plus de `map<int, noeud*>` partageant
//    des pointeurs, source de doubles liberations) ;
//  * la copie est une vraie copie profonde et definie par defaut ;
//  * l'ecriture/lecture du fichier texte est verifiee (l'ancien code ne testait
//    jamais `fopen` ni `fscanf`).
//
// Le format texte lu est identique a celui produit par
// `graphe::ecrire_fichier()` :  nbre_de_sommets, puis les etiquettes, les x,
// les y, puis la matrice.  Les bases deja produites par l'application de 2006
// restent donc lisibles.
#ifndef NIS_CORE_GRAPH_H
#define NIS_CORE_GRAPH_H

#include <string>
#include <vector>

#include "core/Types.h"

namespace nis {

class Graphe {
 public:
  using Matrice = std::vector<std::vector<double>>;

  Graphe() = default;
  explicit Graphe(std::vector<Sommet> sommets, std::string nom = std::string());

  // --- Acces -------------------------------------------------------------
  const std::vector<Sommet>& sommets() const { return sommets_; }
  std::size_t nbSommets() const { return sommets_.size(); }
  const Sommet& sommet(std::size_t indice) const;

  const std::string& nom() const { return nom_; }
  void setNom(std::string nom) { nom_ = std::move(nom); }

  // --- Modification ------------------------------------------------------
  void setSommets(std::vector<Sommet> sommets);
  void ajouterSommet(const Sommet& sommet);
  /// Fournit une matrice explicite (cas du graphe median, dont les poids ne
  /// sont PAS les distances euclidiennes des sommets).
  bool setMatrice(Matrice matrice);

  /// Recalcule la matrice comme distances euclidiennes (comportement historique).
  void recalculerMatrice();

  // --- Poids -------------------------------------------------------------
  const Matrice& matrice() const { return matrice_; }
  double poids(std::size_t i, std::size_t j) const;
  /// Somme des aretes distinctes (i < j) : sert au cout global et aux tests.
  double sommeAretes() const;

  // --- Serialisation -----------------------------------------------------
  /// Format texte identique a `graphe::ecrire_fichier()` : nbre de sommets,
  /// etiquettes, abscisses, ordonnees, matrice.  L'angle des sommets n'y figure
  /// pas (le format de 2006 ne le contenait pas) : il est porte par l'objet en
  /// memoire et recalcule par l'extraction de minuties.
  std::string versChaine() const;
  static bool depuisChaine(const std::string& texte, Graphe* sortie, std::string* erreur);

  bool ecrireFichier(const std::string& chemin, std::string* erreur = nullptr) const;
  static bool depuisFichier(const std::string& chemin, Graphe* sortie, std::string* erreur = nullptr);

 private:
  std::vector<Sommet> sommets_;
  Matrice matrice_;
  std::string nom_;
};

}  // namespace nis

#endif  // NIS_CORE_GRAPH_H
