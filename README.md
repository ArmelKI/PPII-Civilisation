# Les Incivilisés — Early Civilization

Projet PPII FISE 2025-2026 · Sujet : Civilization (version minimaliste en C)

[[_TOC_]]

---

## 1. Équipe

| Membre | Rôle principal | Périmètre technique |
|---|---|---|
| Maxence ARDITTI | Lead technique | Architecture, carte, CLI, SDL, parser |
| Armel Stephane Novak KI | IA & barbares | Makefile, barbares, IA, extensions |
| Ababacar DIENE | Unités, combat & technologie | Unités, combat, arbre technologique |
| Wassim OUKAZI | Villes, ressources & documentation | Villes, ressources, bâtiments, événements |

---

## 2. Présentation du projet

**Les Incivilisés** est un jeu de stratégie au tour par tour en C, fidèle à la spécification *Early Civilization*. Le joueur développe une civilisation sur une carte générée procéduralement : il fonde des villes, recrute des unités, recherche des technologies et doit atteindre une condition de victoire avant que le temps ou les barbares ne l'en empêchent.

Le projet est entièrement jouable en **mode CLI** (ncurses, affichage ASCII) et en **mode graphique** (SDL2, tuiles colorées, interface pointée). Les deux modes partagent exactement le même moteur de jeu.

---

## 3. Démarrage rapide

```bash
# 1. Installer les dépendances système
make req

# 2. Compiler et lancer en mode CLI
make cli

# 3. Compiler et lancer en mode graphique
make sdl

# 4. Lancer les tests unitaires
make test
```

La graine de génération utilisée est affichée en début de partie dans les logs et dans l'interface. Pour rejouer exactement la même carte :

```bash
./bin/civ --seed 42
```

---

## 4. Prérequis (Requirements)

Le projet nécessite les outils et bibliothèques suivants :

| Dépendance | Usage | Installation (Debian/Ubuntu) |
|---|---|---|
| **GCC** | Compilation | `sudo apt install build-essential` |
| **SDL2** | Graphismes | `sudo apt install libsdl2-dev` |
| **SDL2_ttf** | Polices | `sudo apt install libsdl2-ttf-dev` |
| **SDL2_image**| Images | `sudo apt install libsdl2-image-dev` |
| **Ncurses** | Mode CLI | `sudo apt install libncursesw5-dev` |

Pour tout installer d'un coup :
```bash
make req
```

---

## 5. Compilation et cibles Makefile

| Cible | Description |
|---|---|
| `make` | Compile le projet (binaire CLI `bin/civ`) |
| `make cli` | Compile puis lance en mode CLI |
| `make sdl` | Compile avec SDL2 puis lance en mode graphique |
| `make test` | Compile et exécute les tests unitaires |
| `make clean` | Supprime les fichiers de compilation et les exécutables |
| `make reset` | Supprime également les données (sauvegardes, scores, config) |

> Le mode SDL requiert une compilation dédiée (`make sdl`) ; `make` seul produit uniquement le binaire CLI.

---

## 6. Paramètres de lancement

Le programme accepte les options suivantes en ligne de commande. Les deux modes (CLI et SDL) partagent les mêmes paramètres.

| Option longue | Forme courte | Défaut | Description |
|---|:---:|:---:|---|
| `--help` | — | — | Affiche l'aide et quitte |
| `--mode MODE` | `-m` | `cli` | Mode d'affichage : `cli` ou `sdl` |
| `--width W` | `-W` | 50 | Largeur de la carte en tuiles |
| `--height H` | `-H` | 30 | Hauteur de la carte en tuiles |
| `--seed N` | `-s` | aléatoire | Graine de génération de la carte |
| `--turns N` | `-t` | 200 | Nombre de tours maximum |
| `--barbarians N` | `-b` | 3 | Nombre de campements barbares initiaux |
| `--difficulty DIFF` | `-d` | `normal` | Preset : `facile`, `normal`, `difficile` |
| `--production-multiplier X` | `-p` | 1.0 | Multiplicateur de production |
| `--growth-multiplier X` | `-g` | 1.0 | Multiplicateur du seuil de croissance |
| `--barbarian-spawn-multiplier X` | `-B` | 1.0 | Multiplicateur de spawn barbare |
| `--famine on/off` | `-f` | `on` | Active ou désactive la famine |

Si un paramètre invalide est fourni, le programme affiche l'aide (`--help`) et se termine proprement.

**Exemples :**

```bash
./bin/civ --mode sdl --width 40 --height 25 --seed 12345
./bin/civ -m cli -t 150 -b 5 -d difficile
./bin/civ --production-multiplier 0.5 --barbarian-spawn-multiplier 2.0
```

---

## 7. Contrôles — mode CLI (ncurses)

| Touche | Action |
|---|---|
| Clic / Entrée | Sélectionner une unité ou une ville |
| `z` `q` `s` `d` | Déplacer l'unité sélectionnée |
| `f` | Fonder une ville (colon) |
| `c` / `v` | Gérer la ville (construction / recrutement) |
| `t` | Ouvrir l'arbre des technologies |
| `n` | Terminer le tour |
| `Maj+S` | Sauvegarder la partie |
| `x` | Quitter |

La liste complète est accessible via le menu **Règles & commandes** en jeu.

---

## 8. Contrôles — mode SDL

| Touche / Action | Effet |
|---|---|
| Clic gauche | Sélectionner une unité ou une ville |
| `z` `q` `s` `d` | Déplacer l'unité sélectionnée |
| Flèches directionnelles | Déplacer la caméra |
| `v` | Ouvrir le menu de ville |
| `t` | Ouvrir l'arbre des technologies |
| `n` | Terminer le tour |
| `Maj+S` | Sauvegarde manuelle |
| `Échap` | Retour au menu (sauvegarde auto dans `current_game.tmp`) |
| `F11` | Plein écran |

---

## 9. Fonctionnalités réalisées (socle obligatoire)

- Génération de carte par **bruit de Perlin**, graine reproductible, 6 types de terrain.
- **Gestion des villes** : fondation par colon, croissance, famine, production, bâtiments, murailles.
- **Unités** : colon, guerrier, archer, cavalier ; combats automatiques contre unités et villes.
- **Ressources globales** : nourriture, production, science, or ; entretien en or avec dissolution d'unité en cas de découvert.
- **Arbre technologique** en graphe orienté acyclique (9 technologies, prérequis multiples).
- **Barbares** : campements, spawn progressif plafonné, pathfinding BFS, priorité des cibles (colon > guerrier > ville).
- **Conditions de victoire/défaite** et calcul du score de fin de partie.
- Mode CLI (ncurses) et mode SDL2 **partageant le même moteur de jeu**.

---

## 10. Extensions réalisées

| Extension | Description |
|---|---|
| **EXT-1** Navigation & Carte | Technologie **Bateaux** (traversée maritime, coût 2 PM), génération Perlin avec continents cohérents |
| **EXT-2** Merveilles & Ressources | 5 Merveilles du monde uniques, 5 ressources spéciales conditionnées par technologies |
| **EXT-4** Gestion de partie | Menu principal, sauvegarde/chargement (`.sav`, `.tmp`), historique des scores |
| **EXT-5** Difficulté & Unités avancées | Presets Facile/Normal/Difficile, multiplicateurs configurables, unités **Archer** et **Cavalier** |
| **EXT-6** Brouillard de guerre | Statut par tuile : inconnue / explorée / visible ; champ de vision unités et villes |

---

## 11. Séquence d'un tour

| Étape | Description |
|:---:|---|
| 1 | **Restauration des PM** — toutes les unités (joueur et barbares) récupèrent leur maximum |
| 2 | **Revenus des villes** — chaque ville exploite ses tuiles et accumule nourriture, production, science, or |
| 3 | **Croissance** — mise à jour de la population (croissance ou famine) |
| 4 | **Production** — avancement des projets en cours (bâtiments, unités) |
| 5 | **Entretien** — le joueur paye l'or d'entretien global ; en cas de découvert, une unité est dissoute |
| 6 | **Recherche** — avancement de la technologie en cours |
| 7 | **Phase barbares** — spawn depuis les campements, déplacement et attaques |
| 8 | **Fin de tour** — incrémentation du compteur, vérification victoire/défaite |

---

## 12. Unités

| Unité | Coût | Entretien | PV | Attaque | Défense | PM | Prérequis |
|---|:---:|:---:|:---:|:---:|:---:|:---:|---|
| Colon | 50 | 0 | 10 | 0 | 1 | 2 | — |
| Guerrier | 40 | 1 | 15 | 3 | 2 | 3 | Artisanat |
| Archer | 55 | 1 | 10 | 4 | 1 | 2 | Chasse + Caserne |
| Cavalier | 60 | 2 | 15 | 3 | 3 | 4 | Équitation + Caserne |

**Formule de combat :**

```
Dommages → cible    = max(1, Attaque_attaquant − Défense_cible)
Dommages → attaquant = max(0, Attaque_cible − Défense_attaquant)
```

Une ville se défend avec une force de `20` (ou `30` avec muraille). Si ses PV tombent à 0, elle est rasée.

---

## 13. Arbre technologique

| Technologie | Coût | Prérequis | Effet principal |
|---|:---:|---|---|
| Chasse | 50 | — | +1 nourriture sur les tuiles Forêt |
| Agriculture | 60 | — | +10 % nourriture globale |
| Artisanat | 70 | — | Débloque Guerrier, +10 % production |
| Écriture | 80 | — | Débloque Bibliothèque, +10 % science |
| Équitation | 100 | — | Débloque Cavalier |
| Irrigation | 90 | Agriculture | +20 % nourriture supplémentaire |
| Maçonnerie | 100 | Artisanat | Débloque Muraille |
| Commerce | 90 | Artisanat + Écriture | Débloque Marché, +10 % or |
| Bateaux | 110 | Commerce | Navigation maritime (2 PM/tuile eau) |

```
Chasse    Agriculture    Artisanat    Écriture    Équitation
               |           |    \         |
           Irrigation  Maçon.  Commerce ←←←←
                               └──────────→ Bateaux
```

---

## 14. Ressources spéciales (EXT-2)

Environ 15 % des tuiles éligibles portent une ressource spéciale, productive uniquement si la technologie requise est débloquée.

| Ressource | Terrain | Technologie requise | Bonus |
|---|---|---|---|
| Blé | Plaine | Agriculture | +3 nourriture/tour |
| Fer | Montagne | Artisanat | +3 production/tour |
| Épices | Forêt, Désert | Commerce | +4 or/tour |
| Spécimens | Toundra | Écriture | +3 science/tour |
| Chevaux | Plaine, Toundra | Équitation | +1 PM sur les unités produites dans la ville |

---

## 15. Merveilles du monde (EXT-2)

Chaque Merveille est unique dans la partie. Une fois construite dans une ville, son effet est global et immédiat.

| Merveille | Terrain requis | Coût | Effet |
|---|---|:---:|---|
| Pyramides | Désert | 200 | +50 % nourriture dans toutes les villes |
| Grande Bibliothèque | Toundra | 250 | Débloque immédiatement la technologie en cours |
| Colisée | Plaine | 200 | +2 or/tour par ville |
| Tour de Babel | Montagne | 300 | −25 % coût de toutes les futures technologies |
| Jardins suspendus | Forêt | 220 | −20 % seuil de croissance de toutes les villes |

---

## 16. Conditions de victoire, défaite et score

### Victoire

| Condition | Règle |
|---|---|
| Scientifique | Débloquer les 9 technologies de l'arbre |
| Territoriale | Posséder ≥ 10 villes pendant 5 tours consécutifs |

### Défaite

| Condition | Règle |
|---|---|
| Extinction | Plus aucune ville active |
| Temps écoulé | Limite de tours atteinte (`--turns`, défaut 200) |
| Faillite | Production totale nulle pendant 5 tours consécutifs |

### Score

```
score = (nb_technologies × 100)
      + (nb_villes × 50)
      + (population_totale × 20)
      + max(0, (tours_max − tour_actuel) × 10)
```

---

## 17. Presets de difficulté (EXT-5)

| Paramètre | Facile | Normal | Difficile |
|---|:---:|:---:|:---:|
| Multiplicateur de production | 1.25 | 1.0 | 0.75 |
| Multiplicateur seuil de croissance | 0.75 | 1.0 | 1.25 |
| Multiplicateur spawn barbare | 0.75 | 1.0 | 1.25 |
| Famine | désactivée | activée | activée |

Les valeurs exactes sont dans [docs/CONFIG.md](docs/CONFIG.md).

---

## 18. Architecture

```
include/core/       En-têtes publics (types.h, game.h, map.h, barbares.h, …)
src/core/
  game.c            Moteur central : init, next_turn, victoire/défaite, score
  map.c             Génération Perlin, structures de carte
  barbares.c        Spawn, pathfinding BFS, comportement, campements
  ressources.c      Revenus, production, croissance, entretien
  tech.c            Arbre technologique, recherche, prérequis
  physics.c         Combat, déplacement, PM
  config.c          Parser CLI, presets de difficulté
  main.c            Interface CLI (ncurses)
  engine_sdl.c      Interface SDL2
tests/              Tests unitaires (6 suites, ~20 cas)
docs/               Gestion de projet (charte, Gantt, CR réunions)
assets/             Police et images pour le mode SDL
```

Le moteur de jeu (`src/core/game.c` et modules associés) est **strictement découplé** de l'affichage : CLI et SDL partagent les mêmes règles, les mêmes structures et le même `GameState`.

---

## 19. Tests unitaires

```bash
make test
```

| Fichier | Ce qui est testé |
|---|---|
| [tests/test_barbares.c](tests/test_barbares.c) | Init camps, spawn, déplacement, pathfinding BFS, combat, destruction de ville, intégration |
| [tests/test_physique.c](tests/test_physique.c) | Formules de combat, déplacement sur terrain, coûts PM |
| [tests/test_archer.c](tests/test_archer.c) | Statistiques Archer, prérequis technologie Chasse |
| [tests/test_cavalier.c](tests/test_cavalier.c) | Statistiques Cavalier, prérequis technologie Équitation |
| [tests/test_config.c](tests/test_config.c) | Lecture et application des presets de difficulté |
| [tests/test_difficulty_effects.c](tests/test_difficulty_effects.c) | Effets des multiplicateurs sur la production et la croissance |

---

## 20. Sauvegardes et scores

| Fichier | Rôle |
|---|---|
| `savegame.sav` | Sauvegarde manuelle (`Maj+S`) |
| `current_game.tmp` | Sauvegarde automatique de session (écrasée à chaque tour) |
| `score_history.txt` | Historique des scores (mode CLI) |
| `scores.txt` | Historique des scores (mode SDL) |

---

## 21. Documents de gestion de projet

| Document | Lien |
|---|---|
| Charte de projet | [docs/charte_projet_incivilises-1.pdf](docs/charte_projet_incivilises-1.pdf) |
| Gantt final | [docs/gantt_civilization_final.pdf](docs/gantt_civilization_final.pdf) |
| Compte rendu N°1 | [docs/Compte_Rendu_de_Réunion_N1__Lancement_du_Projet.pdf](docs/Compte_Rendu_de_Réunion_N1__Lancement_du_Projet.pdf) |
| Compte rendu N°2 | [docs/Compte_rendu_Reunion%202.pdf](docs/Compte_rendu_Reunion%202.pdf) |
| Compte rendu N°3 | [docs/Compte_rendu_N3.pdf](docs/Compte_rendu_N3.pdf) |
| Presets de difficulté | [docs/CONFIG.md](docs/CONFIG.md) |
| État de l'art | [SOTA.md](SOTA.md) |

---

## 22. Limites connues

- Le mode SDL requiert une compilation séparée (`make sdl`) ; la cible `make` seule produit uniquement le binaire CLI.
- L'historique des scores est stocké dans deux fichiers distincts selon le mode : `score_history.txt` (CLI) et `scores.txt` (SDL).
- Le combat des unités barbares avancées (Archer barbare, Cavalier barbare) utilise les mêmes formules que le guerrier de base ; leur comportement de ciblage ne diffère pas.

---

## 23. Crédits et Outils externes

- **Génération procédurale :** Utilisation de la bibliothèque *stb_perlin.h* (créée par **Sean Barrett**, disponible sur [GitHub](https://github.com/nothings/stb)). Cette bibliothèque est intégrée dans le dossier `include/core/`.
- **Assets graphiques :** Spritesheets — [16x16 Mini World Sprites](https://merchant-shade.itch.io/16x16-mini-world-sprites)
    - Licence : **CC0 / Public Domain** (Libre d'utilisation, de modification et de distribution sans attribution requise, mais créditée ici par rigueur professionnelle).