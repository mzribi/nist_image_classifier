# NIST Image Classifier

> **Portage Qt/C++17** du projet MFC *nis* (ENSI 2006) — Classification d'images par graphes de minuties.

Une application de traitement d'image qui extrait des **minuties** (terminaisons et bifurcations) depuis des images de traits (type empreintes, signatures, schémas), construit des **graphes de minuties**, et les **classifie** par k-moyennes avec **graphes médians** comme centroïdes.

---

## 🎯 Fonctionnalités

| Module | Description |
|--------|-------------|
| **Pipeline image** | Binarisation → suppression fond bord → amincissement Zhang-Suen → extraction minuties |
| **Graphes** | Sommets étiquetés (type, x, y, angle) + matrice de poids (distances euclidiennes) |
| **Appariement** | Branch-and-bound optimal, pénalités étiquette/arête/sommet virtuel |
| **Clustering** | K-moyennes sur graphes, centroïdes = graphes médians (itératif, convergent) |
| **Médiane** | Moyenne des poids *réellement observés* par arête (corrige bug 2006) |
| **Interface Qt** | Deux onglets : « Image » (pas à pas) + « Base » (lot + progression + annulation) |

---

## 🏗 Architecture

```
nist_image_classifier/
├── src/
│   ├── core/           # 🧠 Noyau C++17 pur (SANS Qt, testable headless)
│   │   ├── ImageNiveauxGris   # Image 8 bits 1 canal (fond=255, encre=0)
│   │   ├── PipelineImage      # Binarisation, fond, amincissement, minuties
│   │   ├── Types              # TypeMinutie, Sommet, distances
│   │   ├── Graphe             # Graphe + matrice + I/O format legacy 2006
│   │   ├── Appariement        # Branch-and-bound, couts, validité
│   │   ├── EnsembleGraphes    # Collection + tailles M, EXT_ES, alphabet Ls
│   │   ├── KMeans1D           # K-moyennes 1D déterministe (alphabet étiquettes)
│   │   ├── GrapheMedian       # Médiane itérative (graine → apparier → moyennes)
│   │   ├── Classeur           # K-moyennes graphes (initialisation farthest-point)
│   │   └── ConstructeurBase   # Orchestration : images → graphes → classes → disque
│   └── qt/             # 🖥 Interface Qt Widgets (Qt6 ou Qt5)
│       ├── MainWindow        # Fenêtre principale + onglets + menus
│       ├── PanelImage        # Onglet/Dialogue : traitement image pas à pas
│       ├── PanelBase         # Onglet/Dialogue : classification base (threadé)
│       ├── TravailleurBase   # Worker QThread + progression + annulation
│       ├── ImageQt           # Conversion QImage ↔ ImageNiveauxGris
│       └── VueImage          # Affichage image + minuties (souris, zoom)
├── tests/              # ✅ Tests unitaires (harnais maison, 0 dépendance)
└── CMakeLists.txt      # Build: lib statique + exe UI + exe tests
```

---

## 🐛 Bugs historiques corrigés (2006 → 2024)

| Composant | Bug 2006 | Correction |
|-----------|----------|------------|
| `Classement()` | `if (erreur > min)` gardait la classe **la plus éloignée** | `if (erreur < min)` + initialisation farthest-point |
| `Appariement` | Renvoyait le **MAX** des couts (`cout1 > cout2`) | Branch-and-bound → **minimum global garanti** |
| `Appariement` | Sommet virtuel coût **0** (arete gratuite) | Pénalité `pénalite_arete=50` + `pénalite_sommet=100` |
| `GrapheMedian` | **Un seul poids** recopié pour toutes les arêtes (`P[TAB[0][0]][TAB[1][0]]`) | Moyenne **par arête** des poids observés |
| `GrapheMedian` | `EXT_ES[k] == NULL` (entier vs pointeur) | `SommetVirtuel = -1` explicite |
| `KMeans1D` | Moyenne **entière** (`som/k`) → centroïdes écrasés vers 0 | `double` + arrondi final |
| `KMeans1D` | Boucle `do {} while(!fini)` **infinie** sur égalité | `max_iterations` borne dure |
| Pipeline | `image[i+1][j+1]` **hors limites** sur bords | Accès `au(x,y)` → renvoie `Fond` si hors cadre |
| Pipeline | `while(1)` sans garantie d'arrêt (amincissement) | Marquer puis supprimer, `max_passes` garde-fou |
| Pipeline | Zones codées en dur `[80..200]`, max **5 minuties** | Image entière, pas de limite |
| Mémoire | `new[]` / `delete` (pas `delete[]`), fuites, double-free | `std::vector`, RAII, copie profonde par défaut |
| Chemins | `c:\\Users\\...\\a N\\graphe_median.txt` **en dur** | Dossier de sortie paramétrable |

---

## 📦 Prérequis

| Outil | Version | Notes |
|-------|---------|-------|
| **CMake** | ≥ 3.16 | `cmake --version` |
| **Compilateur C++** | C++17 | GCC ≥ 7, Clang ≥ 5, MSVC ≥ 19.14 |
| **Qt** | **Qt6** (recommandé) ou **Qt5** ≥ 5.12 | `qtbase5-dev` / `qt6-base-dev` |
| **make/ninja** | — | `make -j$(nproc)` |

### Installation dépendances (Ubuntu/Debian)

```bash
# Qt6 (recommandé)
sudo apt-get update && sudo apt-get install -y cmake g++ qt6-base-dev qt6-base-dev-tools

# OU Qt5
sudo apt-get update && sudo apt-get install -y cmake g++ qtbase5-dev qtbase5-dev-tools
```

---

## 🔨 Construction

### 1. Noyau + Tests (sans UI Qt)

```bash
mkdir build && cd build
cmake .. -DNIS_BUILD_UI=OFF -DNIS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 2. Avec interface Qt (Qt6 détecté en priorité)

```bash
mkdir build && cd build
cmake .. -DNIS_BUILD_UI=ON -DNIS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 3. Avec sanitizers (Address + Undefined)

```bash
cmake .. -DNIS_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Artefacts produits

```
build/
├── libnis_core.a           # Bibliothèque statique (noyau)
├── nis                     # Exécutable Qt (si NIS_BUILD_UI=ON)
└── nis_tests               # Exécutable tests (si NIS_BUILD_TESTS=ON)
```

---

## 🧪 Tests

```bash
# Tous les tests
./nis_tests

# Filtre par nom (ex: graphe, appariement, pipeline...)
./nis_tests graphe
./nis_tests median
./nis_tests kmeans
```

### Suites de tests (30+ cas)

| Fichier | Couverture |
|---------|------------|
| `tst_pipeline.cpp` | Binarisation, fond, amincissement, minuties, poids euclidiens |
| `tst_graphe.cpp` | Matrice, sérialisation (format 2006), I/O fichier, validation |
| `tst_appariement.cpp` | Identiques, permutation, étiquettes, virtuels, invalides, limite noeuds |
| `tst_classeur.cpp` | 2 groupes, centre le plus proche (anti-régression), déterministe, bornes |
| `tst_median.cpp` | Identiques, moyennes par arête, taille médiane, annulation |
| `tst_ensemble.cpp` | M, fenêtres, EXT_ES, alphabet, reproductibilité, vide |
| `tst_kmeans.cpp` | 2 étiquettes, k>distinct, convergence, vide, déterministe |
| `tst_base.cpp` | Construction base bout-en-bout (images synthétiques, /tmp, nettoyage) |

---

## 🚀 Utilisation

### Interface graphique

```bash
./nis
```

**Deux onglets / dialogues modaux :**

1. **Image** — Traitement pas à pas d'une image unique :
   - Ouvrir → Binarisation (seuil réglable) → Élimination fond → Miniaturisation → Extraction minuties → Classification (si centres dispo) → Enregistrer
   - Affichage image + minuties superposées à chaque étape

2. **Base de données** — Classification par lot :
   - Choisir extensions (JPG/PNG/BMP/PPM/TIF), dossier images, nb classes (2–6)
   - « Continuer » → choisir dossier sortie → progression temps réel + **Annuler**
   - Résultats en tableau (image, classe, erreur, nb sommets)
   - Génère `<sortie>/classe N/graphe_median.txt` (format legacy) + copie images
   - Centres propagés vers l'onglet Image pour classification isolée

### API programme (noyau seul)

```cpp
#include "core/ConstructeurBase.h"
#include "core/PipelineImage.h"

// 1. Charger une image (implémentation fournie par l'appelant)
nis::ChargeurImage monChargeur = [](const std::string& chemin,
                                    nis::ImageNiveauxGris* img,
                                    std::string* err) -> bool {
    // ... lecture fichier -> remplir *img
    return true;
};

// 2. Construire la base
nis::ConstructeurBase builder(nis::OptionsBase{}, monChargeur);
std::vector<std::string> fichiers = {"img1.png", "img2.png", ...};
std::string dossierSortie = "/chemin/vers/sortie";

auto resultat = builder.construire(fichiers, dossierSortie,
    [](double progression, const std::string& label) {
        std::cout << int(progression*100) << "% - " << label << "\n";
        return true; // false = annuler
    });

// 3. Récupérer les centres pour classifier une nouvelle image
std::vector<nis::Graphe> centres = resultat.centres();
```

---

## 📁 Format de fichier graphe (legacy 2006 compatible)

```text
3                    # nombre de sommets
0 1 0                # étiquettes (0=terminaison, 1=bifurcation)
10 20 30             # abscisses (x)
10 20 10             # ordonnées (y)
0 11 22              # matrice poids (symétrique, diagonale 0)
11 0 33
22 33 0
```

- **Lu/écrit** par `Graphe::depuisFichier()` / `ecrireFichier()`
- **Angles** non stockés (recalculés à l'extraction)
- Compatible avec les bases produites par l'application MFC 2006

---

## ⚙️ Configuration CMake

| Option | Défaut | Description |
|--------|--------|-------------|
| `NIS_BUILD_UI` | `ON` | Construire l'exécutable Qt `nis` |
| `NIS_BUILD_TESTS` | `ON` | Construire et enregistrer `nis_tests` (CTest) |
| `NIS_SANITIZE` | `OFF` | Activer `-fsanitize=address,undefined` (Debug) |
| `CMAKE_BUILD_TYPE` | `Release` | `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |

### Détection Qt

```cmake
# Ordre de préférence :
find_package(Qt6 COMPONENTS Widgets)  # 1. Qt6
find_package(Qt5 5.12 COMPONENTS Widgets)  # 2. Qt5 ≥ 5.12
```

---

## 🔄 Intégration Continue (GitLab CI)

`.gitlab-ci.yml` inclus :
- **SAST** (Static Application Security Testing)
- **Secret Detection**
- Stages: `test` → `secret-detection`

```yaml
stages:
  - test
  - secret-detection
sast:
  stage: test
  include:
    - template: Security/SAST.gitlab-ci.yml
secret_detection:
  stage: secret-detection
  variables:
    SECRET_DETECTION_ENABLED: 'true'
```

---

## 📜 Licence

Projet étudiant **ENSI 2006** — Portage moderne **2024**.
Code source : usage éducatif / recherche.

---

## 👥 Auteurs

- **Projet original (2006)** : *Deux Modules, ENSI* — Code MFC (`ClassificationImage.cpp`, `graphe.cpp`, `appariement.cpp`, `classificateur.cpp`, `graphe_median.cpp`, `k_means.cpp`, `ens_graph.cpp`, `image.cpp`)
- **Portage Qt/C++17 (2024)** : Réécriture complète du noyau, correction des bugs, tests unitaires, interface Qt Widgets, CMake, CI

---

## 📚 Références algorithmiques

- **Amincissement** : Zhang & Suen (1984) — *A Fast Parallel Algorithm for Thinning Digital Patterns*
- **Minuties** : Terminaison (1 voisin), Bifurcation (3+ voisins hors bloc 2×2)
- **Appariement** : Branch-and-bound sur injections partielles (coûts additifs, borne admissible)
- **Médiane de graphes** : Itératif — graine → apparier → moyennes par créneau → stabilisation
- **K-moyennes 1D** : Lloyd déterministe, initialisation échantillonnage régulier sur alphabet trié

---

## 🛠 Développement

### Ajouter un test

```cpp
// tests/tst_monmodule.cpp
#include "Harness.h"
#include "core/MonModule.h"

NIS_TEST(monmodule_fonctionnalite_x) {
    NIS_VERIF(condition);
    NIS_VERIF_EQ(valeur, attendue);
    NIS_VERIF_PROCHE(flottant, attendu, 1e-9);
}
```
Reconstruire : `cd build && make nis_tests && ./nis_tests monmodule`

### Style de code

- C++17, pas d'extensions (`-std=c++17 -pedantic`)
- Warnings : `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`
- Nommage : `snake_case` (fonctions/variables), `PascalCase` (types), `UPPER_SNAKE` (constantes)
- Noyau **sans Qt** : `#include <...>` seulement, pas de `Q_*` dans `src/core/`

---

## 📊 Métriques

| Métrique | Valeur |
|----------|--------|
| Lignes noyau (core) | ~3 500 |
| Lignes UI (qt) | ~2 500 |
| Lignes tests | ~2 000 |
| Couverture fonctionnelle | Pipeline, Appariement, Clustering, Médiane, KMeans, Ensemble, Base |
| Dépendances noyau | **0** (std:: seulement) |
| Dépendances UI | Qt Widgets (Qt5/6) |
