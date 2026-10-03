#include "core/Graphe.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace nis {
namespace {

/// Formate un nombre sans decimale inutile : un fichier ecrit par le nouveau
/// code reste identique, octet pour octet, a un fichier de 2006 quand les
/// valeurs sont entieres.
std::string formater(double valeur) {
  if (std::isfinite(valeur) && valeur == std::floor(valeur) &&
      std::fabs(valeur) < 1e15) {
    std::ostringstream flux;
    flux << static_cast<long long>(std::llround(valeur));
    return flux.str();
  }
  std::ostringstream flux;
  flux.precision(6);
  flux << std::fixed << valeur;
  return flux.str();
}

bool lireEnTiers(std::istream& flux, long long attendu, const char* quoi,
                 std::vector<double>* sortie, std::string* erreur) {
  sortie->clear();
  sortie->reserve(static_cast<std::size_t>(attendu));
  for (long long i = 0; i < attendu; ++i) {
    double valeur = 0.0;
    if (!(flux >> valeur)) {
      if (erreur) *erreur = std::string("valeur manquante pour ") + quoi;
      return false;
    }
    sortie->push_back(valeur);
  }
  return true;
}

}  // namespace

Graphe::Graphe(std::vector<Sommet> sommets, std::string nom)
    : sommets_(std::move(sommets)), nom_(std::move(nom)) {
  recalculerMatrice();
}

const Sommet& Graphe::sommet(std::size_t indice) const {
  if (indice >= sommets_.size())
    throw std::out_of_range("Graphe::sommet : indice hors limites");
  return sommets_[indice];
}

void Graphe::setSommets(std::vector<Sommet> sommets) {
  sommets_ = std::move(sommets);
  recalculerMatrice();
}

void Graphe::ajouterSommet(const Sommet& sommet) {
  sommets_.push_back(sommet);
  recalculerMatrice();
}

bool Graphe::setMatrice(Matrice matrice) {
  const std::size_t n = sommets_.size();
  if (matrice.size() != n) return false;
  for (const std::vector<double>& ligne : matrice) {
    if (ligne.size() != n) return false;
  }
  matrice_ = std::move(matrice);
  return true;
}

void Graphe::recalculerMatrice() {
  const std::size_t n = sommets_.size();
  matrice_.assign(n, std::vector<double>(n, 0.0));
  for (std::size_t i = 0; i < n; ++i) {
    matrice_[i][i] = 0.0;
    for (std::size_t j = i + 1; j < n; ++j) {
      const double d = distanceEuclidienne(sommets_[i], sommets_[j]);
      matrice_[i][j] = d;
      matrice_[j][i] = d;  // la matrice historique etait symetrique
    }
  }
}

double Graphe::poids(std::size_t i, std::size_t j) const {
  if (i >= matrice_.size() || j >= matrice_[i].size())
    throw std::out_of_range("Graphe::poids : indice hors limites");
  return matrice_[i][j];
}

double Graphe::sommeAretes() const {
  double somme = 0.0;
  const std::size_t n = nbSommets();
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = i + 1; j < n; ++j) somme += poids(i, j);
  return somme;
}

std::string Graphe::versChaine() const {
  const std::size_t n = sommets_.size();
  std::ostringstream flux;
  flux << n << '\n';
  for (std::size_t i = 0; i < n; ++i)
    flux << static_cast<int>(sommets_[i].type) << ' ';
  flux << '\n';
  for (std::size_t i = 0; i < n; ++i) flux << formater(sommets_[i].x) << ' ';
  flux << '\n';
  for (std::size_t i = 0; i < n; ++i) flux << formater(sommets_[i].y) << ' ';
  flux << '\n';
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) flux << formater(poids(i, j)) << ' ';
    flux << '\n';
  }
  return flux.str();
}

bool Graphe::depuisChaine(const std::string& texte, Graphe* sortie, std::string* erreur) {
  if (sortie == nullptr) return false;
  if (erreur) erreur->clear();

  std::istringstream flux(texte);
  long long n = -1;
  if (!(flux >> n)) {
    if (erreur) *erreur = "fichier de graphe vide ou corrompu";
    return false;
  }
  if (n < 0 || n > 10000) {
    if (erreur) *erreur = "nombre de sommets impossible : " + std::to_string(n);
    return false;
  }

  std::vector<double> etiquettes, abscisses, ordonnees;
  if (!lireEnTiers(flux, n, "etiquettes", &etiquettes, erreur)) return false;
  if (!lireEnTiers(flux, n, "coordonnees x", &abscisses, erreur)) return false;
  if (!lireEnTiers(flux, n, "coordonnees y", &ordonnees, erreur)) return false;

  const std::size_t taille = static_cast<std::size_t>(n);
  Matrice matrice(taille, std::vector<double>(taille, 0.0));
  for (long long i = 0; i < n; ++i) {
    for (long long j = 0; j < n; ++j) {
      double valeur = 0.0;
      if (!(flux >> valeur)) {
        if (erreur) *erreur = "matrice incomplete";
        return false;
      }
      matrice[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = valeur;
    }
  }

  std::vector<Sommet> sommets;
  sommets.reserve(taille);
  for (long long i = 0; i < n; ++i) {
    const int etiq = static_cast<int>(std::lround(etiquettes[static_cast<std::size_t>(i)]));
    if (etiq < 0 || etiq > 1) {
      if (erreur) *erreur = "etiquette de minutie invalide : " + std::to_string(etiq);
      return false;
    }
    Sommet s;
    s.type = (etiq == 0) ? TypeMinutie::Terminaison : TypeMinutie::Bifurcation;
    s.x = abscisses[static_cast<std::size_t>(i)];
    s.y = ordonnees[static_cast<std::size_t>(i)];
    sommets.push_back(s);
  }

  Graphe g(std::move(sommets));
  if (!g.setMatrice(std::move(matrice))) {
    if (erreur) *erreur = "matrice incoherente avec le nombre de sommets";
    return false;
  }
  *sortie = std::move(g);
  return true;
}

bool Graphe::ecrireFichier(const std::string& chemin, std::string* erreur) const {
  std::ofstream flux(chemin, std::ios::out | std::ios::trunc);
  if (!flux) {
    if (erreur) *erreur = "impossible d'ecrire le fichier : " + chemin;
    return false;
  }
  flux << versChaine();
  flux.flush();
  if (!flux) {
    if (erreur) *erreur = "erreur d'ecriture disque : " + chemin;
    return false;
  }
  return true;
}

bool Graphe::depuisFichier(const std::string& chemin, Graphe* sortie, std::string* erreur) {
  std::ifstream flux(chemin, std::ios::in);
  if (!flux) {
    if (erreur) *erreur = "impossible de lire le fichier : " + chemin;
    return false;
  }
  std::ostringstream tampon;
  tampon << flux.rdbuf();
  const bool ok = depuisChaine(tampon.str(), sortie, erreur);
  if (ok && sortie != nullptr) sortie->setNom(chemin);
  return ok;
}

}  // namespace nis

