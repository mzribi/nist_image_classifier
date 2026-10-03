// Tests du ConstructeurBase : chaine fichiers -> graphes -> classes -> disque.
// Le chargeur d'image est injecte (formes synthetiques), sans Qt ni fichier
// reel : seuls les dossiers de sortie sont crees, dans /tmp, puis nettoyes.
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "Harness.h"
#include "core/ConstructeurBase.h"

namespace {

void tracerLigne(nis::ImageNiveauxGris* img, int x0, int y0, int x1, int y1) {
  int dx = (x1 > x0) ? 1 : -1;
  int dy = (y1 > y0) ? 1 : -1;
  int x = x0, y = y0;
  img->set(static_cast<std::size_t>(x), static_cast<std::size_t>(y),
           nis::ImageNiveauxGris::Encre);
  while (x != x1 || y != y1) {
    if (x != x1) x += dx;
    if (y != y1) y += dy;
    img->set(static_cast<std::size_t>(x), static_cast<std::size_t>(y),
             nis::ImageNiveauxGris::Encre);
  }
}

/// Deux familles de formes : croix fine (classe attendue A) et grand carre
/// (classe attendue B).  Le nom du fichier choisit la famille.
nis::ImageNiveauxGris formeSynthetique(const std::string& nom) {
  nis::ImageNiveauxGris img(40, 40, nis::ImageNiveauxGris::Fond);
  if (nom.find("carre") != std::string::npos) {
    tracerLigne(&img, 5, 5, 34, 5);
    tracerLigne(&img, 34, 5, 34, 34);
    tracerLigne(&img, 34, 34, 5, 34);
    tracerLigne(&img, 5, 34, 5, 5);
  } else {
    tracerLigne(&img, 20, 4, 20, 35);
    tracerLigne(&img, 4, 20, 35, 20);
  }
  return img;
}

nis::ChargeurImage chargeurSynthetique() {
  return [](const std::string& chemin, nis::ImageNiveauxGris* sortie,
            std::string* erreur) {
    if (sortie == nullptr) return false;
    if (chemin.find("illisible") != std::string::npos) {
      if (erreur) *erreur = "fichier corrompu (simule)";
      return false;
    }
    *sortie = formeSynthetique(chemin);
    return true;
  };
}

nis::OptionsBase optionsBaseTest() {
  nis::OptionsBase o;
  o.classification.nb_classes = 2;
  o.classification.max_iterations = 20;
  o.pipeline.distance_minimale = 0;
  o.copier_images = false;  // rien a copier : les fichiers n'existent pas
  return o;
}

std::string dossierTemporaire(const std::string& suffixe) {
  namespace fs = std::filesystem;
  const fs::path base = fs::temp_directory_path() / ("nis-test-" + suffixe);
  std::error_code ec;
  fs::remove_all(base, ec);
  return base.string();
}

int rangClasse(const nis::ResultatBase& r, std::size_t entree) {
  return r.images.at(entree).classe;
}

}  // namespace

NIS_TEST(base_classe_deux_familles_et_ecrit_les_medians) {
  const std::vector<std::string> fichiers = {"croix_1", "croix_2", "carre_1", "carre_2"};
  double derniere_fraction = -1.0;
  int appels = 0;
  nis::ConstructeurBase b(optionsBaseTest(), chargeurSynthetique());
  const std::string sortie = dossierTemporaire("familles");
  const nis::ResultatBase r = b.construire(
      fichiers, sortie,
      [&](double f, const std::string&) {
        NIS_VERIF(f >= derniere_fraction);
        derniere_fraction = f;
        ++appels;
        return true;
      });
  NIS_VERIF(!r.annule);
  NIS_VERIF(appels > 0);
  NIS_VERIF_PROCHE(derniere_fraction, 1.0, 1e-9);
  NIS_VERIF_EQ(r.images.size(), 4u);
  NIS_VERIF_EQ(r.classes.size(), 2u);
  NIS_VERIF_EQ(r.graphes_ecrits, 2u);
  NIS_VERIF(r.avertissements.empty());
  NIS_VERIF(rangClasse(r, 0) == rangClasse(r, 1));
  NIS_VERIF(rangClasse(r, 2) == rangClasse(r, 3));
  NIS_VERIF(rangClasse(r, 0) != rangClasse(r, 2));
  for (const nis::ResultatImage& img : r.images) {
    NIS_VERIF(img.classe >= 0);
    NIS_VERIF(img.echec.empty());
    NIS_VERIF(img.graphe.nbSommets() > 0u);
  }
  NIS_VERIF_EQ(r.centres().size(), 2u);
  namespace fs = std::filesystem;
  NIS_VERIF(fs::exists(fs::path(sortie) / "classe 1" / "graphe_median.txt"));
  NIS_VERIF(fs::exists(fs::path(sortie) / "classe 2" / "graphe_median.txt"));
  // Les medians ecrits se relisent au format legacy.
  for (int c = 1; c <= 2; ++c) {
    nis::Graphe relu;
    std::string erreur;
    const std::string chemin = sortie + "/classe " + std::to_string(c) + "/graphe_median.txt";
    NIS_VERIF(nis::Graphe::depuisFichier(chemin, &relu, &erreur));
    NIS_VERIF(relu.nbSommets() > 0u);
  }
  std::error_code ec;
  fs::remove_all(sortie, ec);
}

NIS_TEST(base_signale_sans_interrompre) {
  const std::vector<std::string> fichiers = {"croix_1", "illisible_1", "carre_1"};
  nis::ConstructeurBase b(optionsBaseTest(), chargeurSynthetique());
  const std::string sortie = dossierTemporaire("avertissements");
  const nis::ResultatBase r = b.construire(fichiers, sortie);
  NIS_VERIF(!r.annule);
  NIS_VERIF_EQ(r.images.size(), 3u);
  NIS_VERIF(!r.images[1].echec.empty());
  NIS_VERIF_EQ(r.images[1].classe, -1);
  NIS_VERIF(!r.avertissements.empty());
  NIS_VERIF_EQ(r.classes.size(), 2u);
  std::error_code ec;
  std::filesystem::remove_all(sortie, ec);
}

NIS_TEST(base_honore_la_demande_d_arret) {
  const std::vector<std::string> fichiers = {"croix_1", "croix_2", "carre_1", "carre_2"};
  nis::ConstructeurBase b(optionsBaseTest(), chargeurSynthetique());
  const nis::ResultatBase r = b.construire(
      fichiers, dossierTemporaire("annulation"),
      [](double fraction, const std::string&) { return fraction < 0.3; });
  NIS_VERIF(r.annule);
}
