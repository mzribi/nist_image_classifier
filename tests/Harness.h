// Harnais de tests minimal, sans dependance externe (ni GTest, ni Catch) :
// le projet doit pouvoir etre reconstruit sur une machine neuve avec seulement
// un compilateur C++17 et Qt.
//
//   NIS_TEST(nom_du_test) { NIS_VERIF(...); NIS_VERIF_EQ(...); }
//   ./nis_tests              lance tout
//   ./nis_tests graphe       lance les tests dont le nom contient « graphe »
#ifndef NIS_TESTS_HARNESS_H
#define NIS_TESTS_HARNESS_H

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace test {

struct Enregistrement {
  std::string nom;
  std::function<void()> corps;
};

inline std::vector<Enregistrement>& tests() {
  static std::vector<Enregistrement> liste;
  return liste;
}

inline int& echecs_courants() {
  static int compteur = 0;
  return compteur;
}

struct Enregistrateur {
  Enregistrateur(const std::string& nom, std::function<void()> corps) {
    tests().push_back(Enregistrement{nom, std::move(corps)});
  }
};

template <typename T>
std::string decrire(const T& valeur) {
  std::ostringstream flux;
  flux << valeur;
  return flux.str();
}

inline std::string decrire(bool valeur) { return valeur ? "vrai" : "faux"; }

inline int lancer(int argc, char** argv) {
  const std::string filtre = (argc > 1) ? argv[1] : std::string();
  int executes = 0;
  int echoues = 0;
  for (const Enregistrement& t : tests()) {
    if (!filtre.empty() && t.nom.find(filtre) == std::string::npos) continue;
    ++executes;
    echecs_courants() = 0;
    std::cout << "- " << t.nom << std::flush;
    try {
      t.corps();
    } catch (const std::exception& e) {
      ++echecs_courants();
      std::cout << "\n  EXCEPTION : " << e.what() << "\n";
    } catch (...) {
      ++echecs_courants();
      std::cout << "\n  EXCEPTION inconnue\n";
    }
    if (echecs_courants() == 0) {
      std::cout << " ... ok\n";
    } else {
      std::cout << "        (" << echecs_courants() << " verification(s) en echec)\n";
      ++echoues;
    }
  }
  std::cout << (echoues == 0 ? "\nSUCCES : " : "\nECHEC : ") << executes - echoues << "/"
            << executes << " test(s) reussis\n";
  return echoues == 0 ? 0 : 1;
}

}  // namespace test

#define NIS_TEST(nom)                                                             \
  static void nom();                                                              \
  static ::test::Enregistrateur enregistrement_##nom(#nom, &nom);                 \
  static void nom()

#define NIS_VERIF(cond)                                                           \
  do {                                                                            \
    if (!(cond)) {                                                                 \
      ++::test::echecs_courants();                                                 \
      std::cout << "\n  ECHEC " << __FILE__ << ":" << __LINE__ << " : attendu "    \
                << #cond << "\n";                                                  \
    }                                                                             \
  } while (false)

#define NIS_VERIF_EQ(a, b)                                                        \
  do {                                                                            \
    const auto& valeur_a = (a);                                                   \
    const auto& valeur_b = (b);                                                   \
    if (!(valeur_a == valeur_b)) {                                                 \
      ++::test::echecs_courants();                                                 \
      std::cout << "\n  ECHEC " << __FILE__ << ":" << __LINE__ << " : " << #a      \
                << " == " << #b << "  (obtenu " << ::test::decrire(valeur_a)       \
                << " attendu " << ::test::decrire(valeur_b) << ")\n";              \
    }                                                                             \
  } while (false)

#define NIS_VERIF_PROCHE(a, b, epsilon)                                           \
  do {                                                                            \
    const double valeur_a = static_cast<double>(a);                               \
    const double valeur_b = static_cast<double>(b);                               \
    if (std::fabs(valeur_a - valeur_b) > (epsilon)) {                             \
      ++::test::echecs_courants();                                                 \
      std::cout << "\n  ECHEC " << __FILE__ << ":" << __LINE__ << " : " << #a      \
                << " proche de " << #b << "  (obtenu " << valeur_a << " attendu "   \
                << valeur_b << ")\n";                                              \
    }                                                                             \
  } while (false)

#endif  // NIS_TESTS_HARNESS_H
