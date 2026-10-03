// Tests du Classeur (k-moyennes sur graphes).  Anti-regression du bug 2006
// `Classement()` : `if (erreur > min)` retenait la classe la PLUS ELOIGNEE.
#include <string>
#include <vector>

#include "Harness.h"
#include "core/Classeur.h"

namespace {

/// Grappe compacte de 4 sommets : carre de cote `pas`, decalable en bloc.
nis::Graphe grappe(double ox, double oy, double pas, nis::TypeMinutie type) {
  std::vector<nis::Sommet> s;
  s.push_back(nis::Sommet{type, ox, oy, 0.0});
  s.push_back(nis::Sommet{type, ox + pas, oy, 0.0});
  s.push_back(nis::Sommet{type, ox, oy + pas, 0.0});
  s.push_back(nis::Sommet{type, ox + pas, oy + pas, 0.0});
  return nis::Graphe(s);
}

nis::OptionsClassification optionsDeuxClasses() {
  nis::OptionsClassification o;
  o.nb_classes = 2;
  o.max_iterations = 20;
  return o;
}

/// Quatre graphes, deux groupes bien separes (petit carre a gauche, grand a
/// droite).  L'ordre est melange pour ne pas favoriser l'initialisation.
std::vector<nis::Graphe> deuxGroupes() {
  const auto T = nis::TypeMinutie::Terminaison;
  return {grappe(100.0, 0.0, 30.0, T), grappe(0.0, 0.0, 10.0, T),
          grappe(0.0, 5.0, 12.0, T), grappe(110.0, 2.0, 28.0, T)};
}

}  // namespace

NIS_TEST(classeur_separe_deux_groupes) {
  const std::vector<nis::Graphe> graphes = deuxGroupes();
  const nis::ResultatClassement r = nis::Classeur(optionsDeuxClasses()).classer(graphes);
  NIS_VERIF(!r.annule);
  NIS_VERIF(r.converge);
  NIS_VERIF_EQ(r.classes.size(), 2u);
  NIS_VERIF_EQ(r.affectation.size(), 4u);
  // Les deux petits carres ensemble, les deux grands ensemble, classes
  // distinctes.
  NIS_VERIF(r.affectation[1] == r.affectation[2]);
  NIS_VERIF(r.affectation[0] == r.affectation[3]);
  NIS_VERIF(r.affectation[0] != r.affectation[1]);
  for (const nis::Classe& c : r.classes) {
    NIS_VERIF_EQ(c.membres.size(), 2u);
    NIS_VERIF_EQ(c.median.nbSommets(), 4u);
  }
  // Repertoires historiques.
  NIS_VERIF(nis::ResultatClassement::nomRepertoire(0) == "classe 1");
  NIS_VERIF(nis::ResultatClassement::nomRepertoire(2) == "classe 3");
}

NIS_TEST(classeur_choisit_le_centre_le_plus_proche) {
  // Anti-regression directe du bug `if (erreur > min)` de 2006, qui retenait
  // la classe la PLUS ELOIGNEE.  Note : les poids sont des distances, donc
  // invariants par translation pure ; les deux centres doivent avoir des
  // FORMES differentes (carre 10 contre rectangle 40x10), pas seulement des
  // positions differentes.
  const auto T = nis::TypeMinutie::Terminaison;
  const nis::Graphe centreA = grappe(0.0, 0.0, 10.0, T);  // carre de cote 10
  std::vector<nis::Sommet> rb;
  rb.push_back(nis::Sommet{T, 500.0, 0.0, 0.0});
  rb.push_back(nis::Sommet{T, 540.0, 0.0, 0.0});
  rb.push_back(nis::Sommet{T, 500.0, 10.0, 0.0});
  rb.push_back(nis::Sommet{T, 540.0, 10.0, 0.0});
  const nis::Graphe centreB(rb);  // rectangle 40x10 : forme differente
  // Requete : carre 10 legerement deforme, donc nettement plus proche de A.
  std::vector<nis::Sommet> rq;
  rq.push_back(nis::Sommet{T, 0.0, 0.0, 0.0});
  rq.push_back(nis::Sommet{T, 10.0, 0.0, 0.0});
  rq.push_back(nis::Sommet{T, 0.0, 10.0, 0.0});
  rq.push_back(nis::Sommet{T, 14.0, 15.0, 0.0});  // deforme : (10,10) -> (14,15)
  const nis::Graphe requete(rq);
  const nis::OptionsAppariement opt;
  const double errA = nis::Appariement(requete, centreA, opt).calculer().erreur;
  const double errB = nis::Appariement(requete, centreB, opt).calculer().erreur;
  NIS_VERIF(errA < errB);  // garde-fou : le cas de test est discriminant
  // A est liste en DEUXIEME : il faut quand meme son rang (1), avec son erreur.
  double erreur = -1.0;
  NIS_VERIF_EQ(nis::Classeur::classeLaPlusProche(requete, {centreB, centreA}, opt, &erreur), 1);
  NIS_VERIF_PROCHE(erreur, errA, 1e-9);
  // Et en premier : rang 0, meme erreur.  Une version « plus eloigne » (bug
  // 2006) rendrait ici 1 au lieu de 0.
  NIS_VERIF_EQ(nis::Classeur::classeLaPlusProche(requete, {centreA, centreB}, opt, &erreur), 0);
  NIS_VERIF_PROCHE(erreur, errA, 1e-9);
  NIS_VERIF_EQ(nis::Classeur::classeLaPlusProche(requete, {}, opt, nullptr), -1);
}

NIS_TEST(classeur_est_deterministe) {
  const std::vector<nis::Graphe> graphes = deuxGroupes();
  const nis::Classeur classeur(optionsDeuxClasses());
  const nis::ResultatClassement r1 = classeur.classer(graphes);
  const nis::ResultatClassement r2 = classeur.classer(graphes);
  NIS_VERIF(r1.affectation == r2.affectation);
}

NIS_TEST(classeur_borne_les_classes_au_nombre_de_graphes) {
  const std::vector<nis::Graphe> graphes = deuxGroupes();
  nis::OptionsClassification o = optionsDeuxClasses();
  o.nb_classes = 12;  // plus que de graphes : ramene a 4, sans echec
  const nis::ResultatClassement r = nis::Classeur(o).classer(graphes);
  NIS_VERIF(!r.annule);
  NIS_VERIF_EQ(r.classes.size(), 4u);
  NIS_VERIF_EQ(r.affectation.size(), 4u);
}

NIS_TEST(classeur_entree_vide) {
  const nis::ResultatClassement r = nis::Classeur(optionsDeuxClasses()).classer({});
  NIS_VERIF(r.converge);
  NIS_VERIF(!r.annule);
  NIS_VERIF(r.classes.empty());
}

NIS_TEST(classeur_honore_l_annulation) {
  nis::OptionsClassification o = optionsDeuxClasses();
  o.median.annulation = [] { return false; };
  const nis::ResultatClassement r = nis::Classeur(o).classer(deuxGroupes());
  NIS_VERIF(r.annule);
}
