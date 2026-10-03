#include "core/ConstructeurBase.h"

#include <algorithm>
#include <atomic>
#include <filesystem>

#include "core/Appariement.h"

namespace nis {
namespace {

bool signaler(const Progression& progression, double fraction, const std::string& libelle) {
  if (!progression) return true;
  return progression(fraction, libelle);
}

}  // namespace

ConstructeurBase::ConstructeurBase(OptionsBase options, ChargeurImage chargeur)
    : options_(std::move(options)), chargeur_(std::move(chargeur)) {}

std::vector<Graphe> ResultatBase::centres() const {
  std::vector<Graphe> centres;
  centres.reserve(classes.size());
  for (const Classe& c : classes) centres.push_back(c.median);
  return centres;
}

std::vector<double> ConstructeurBase::erreursParClasse(const Graphe& graphe,
                                                       const std::vector<Graphe>& centres,
                                                       const OptionsAppariement& options) {
  std::vector<double> erreurs;
  erreurs.reserve(centres.size());
  for (const Graphe& centre : centres) {
    erreurs.push_back(Appariement(graphe, centre, options).calculer().erreur);
  }
  return erreurs;
}

ResultatBase ConstructeurBase::construire(const std::vector<std::string>& fichiers,
                                          const std::string& dossier_sortie,
                                          const Progression& progression) const {
  ResultatBase resultat;
  resultat.dossier_sortie = dossier_sortie;
  if (fichiers.empty()) {
    resultat.avertissements.push_back("aucun fichier a traiter");
    return resultat;
  }
  if (!chargeur_) {
    resultat.avertissements.push_back("aucun chargeur d'image fourni");
    return resultat;
  }

  std::atomic<bool> annule{false};
  const auto notifier = [&](double fraction, const std::string& libelle) {
    if (!signaler(progression, fraction, libelle)) {
      annule.store(true);
      return false;
    }
    return true;
  };

  // --- 1. Images -> graphes (0 % -> 60 %).
  // `correspondance[g]` designe l'entree de `resultat.images` du g-ieme graphe :
  // un fichier en echec cree une entree mais pas de graphe, les deux listes ne
  // sont donc pas paralleles.
  std::vector<Graphe> graphes;
  std::vector<std::size_t> correspondance;
  graphes.reserve(fichiers.size());
  correspondance.reserve(fichiers.size());
  resultat.images.reserve(fichiers.size());

  for (std::size_t i = 0; i < fichiers.size(); ++i) {
    if (annule.load()) break;
    const std::string& fichier = fichiers[i];
    const double fraction =
        0.6 * static_cast<double>(i) / static_cast<double>(fichiers.size());
    if (!notifier(fraction, "traitement de " + fichier)) break;

    ResultatImage entree;
    entree.fichier = fichier;

    ImageNiveauxGris image;
    std::string erreur;
    if (!chargeur_(fichier, &image, &erreur) || image.vide()) {
      entree.echec = erreur.empty() ? "image illisible ou vide" : erreur;
      resultat.avertissements.push_back(fichier + " : " + entree.echec);
      resultat.images.push_back(std::move(entree));
      continue;
    }

    const pipeline::ResultatChaine chaine = pipeline::chaineComplete(image, options_.pipeline);
    entree.graphe = chaine.graphe;
    entree.graphe.setNom(fichier);
    if (entree.graphe.nbSommets() == 0) {
      // Graphe vide conserve : il sera classe mais signale, comme l'ancien code
      // qui ignorait silencieusement ces images.
      entree.echec = "aucune minutie extraite";
      resultat.avertissements.push_back(fichier + " : " + entree.echec);
    }
    graphes.push_back(entree.graphe);
    correspondance.push_back(resultat.images.size());
    resultat.images.push_back(std::move(entree));
  }

  if (annule.load()) {
    resultat.annule = true;
    return resultat;
  }
  if (graphes.empty()) {
    resultat.avertissements.push_back("aucune image n'a pu etre convertie en graphe");
    return resultat;
  }

  // --- 2. Classification (60 % -> 90 %).
  if (!notifier(0.6, "classification des graphes")) {
    resultat.annule = true;
    return resultat;
  }
  OptionsBase options = options_;
  options.classification.median.annulation = [&annule]() { return !annule.load(); };
  const ResultatClassement classement = Classeur(options.classification).classer(graphes);
  if (classement.annule) {
    resultat.annule = true;
    return resultat;
  }
  resultat.classes = classement.classes;

  for (std::size_t g = 0; g < graphes.size(); ++g) {
    const int rang = (g < classement.affectation.size()) ? classement.affectation[g] : -1;
    if (rang < 0 || static_cast<std::size_t>(rang) >= resultat.classes.size()) continue;
    ResultatImage& entree = resultat.images[correspondance[g]];
    entree.classe = rang;
    entree.erreur = Appariement(graphes[g], resultat.classes[static_cast<std::size_t>(rang)].median,
                                options.classification.median.appariement)
                        .calculer()
                        .erreur;
  }

  // --- 3. Ecriture sur disque (90 % -> 100 %).
  if (!notifier(0.9, "ecriture des graphes medians")) {
    resultat.annule = true;
    return resultat;
  }
  std::error_code ec;
  std::filesystem::create_directories(dossier_sortie, ec);
  if (ec) {
    resultat.avertissements.push_back("creation du dossier de sortie impossible : " + ec.message());
    return resultat;
  }

  for (std::size_t c = 0; c < resultat.classes.size(); ++c) {
    const std::string dossier = dossier_sortie + "/" + ResultatClassement::nomRepertoire(c);
    std::error_code creation;
    std::filesystem::create_directories(dossier, creation);
    if (creation) {
      resultat.avertissements.push_back("creation de " + dossier + " impossible : " + creation.message());
      continue;
    }
    std::string erreur;
    if (resultat.classes[c].median.ecrireFichier(dossier + "/graphe_median.txt", &erreur)) {
      ++resultat.graphes_ecrits;
    } else {
      resultat.avertissements.push_back(erreur);
    }
    if (!options_.copier_images) continue;
    for (const std::size_t m : resultat.classes[c].membres) {
      if (m >= fichiers.size()) continue;
      const std::string& source = fichiers[m];
      const std::string nom = std::filesystem::path(source).filename().string();
      if (nom.empty()) continue;
      std::error_code copie;
      std::filesystem::copy_file(source, dossier + "/" + nom,
                                 std::filesystem::copy_options::overwrite_existing, copie);
      if (copie) {
        resultat.avertissements.push_back("copie impossible de " + source + " : " + copie.message());
      }
    }
  }

  notifier(1.0, "termine");
  return resultat;
}

}  // namespace nis

