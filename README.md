# PPII — Early Civilization (Les Incivilises)

Projet PPII FISE 2025-2026. Sujet choisi : Civilization (version minimaliste).

[[_TOC_]]

---

## 1. Contrat académique

Ce README contient toutes les informations requises par le sujet :

- membres et rôles ;
- sujet choisi et extensions réalisées ;
- dépendances logicielles ;
- commandes de compilation, lancement et tests ;
- paramètres de lancement supportés ;
- limites connues ;
- liens vers les documents de gestion de projet.

---

## 2. Équipe

| Membre | Rôle principal | Périmètre technique |
|---|---|---|
| Maxence ARDITTI | Lead technique | Architecture, carte, CLI, SDL, parser |
| Armel Stephane Novak KI | IA & barbares | Makefile, barbares, IA, extensions |
| Ababacar DIENE | Unités, combat & technologie | Unités, combat, arbre technologique |
| Wassim OUKAZI | Villes, ressources & documentation | Villes, ressources, bâtiments, événements |

---

## 3. Sujet et objectif

**Early Civilization** est un jeu de stratégie au tour par tour en C, jouable en mode CLI (ncurses) et en mode graphique (SDL2). Le joueur développe sa civilisation — villes, unités, technologies — et affronte les barbares avant que le temps ne soit écoulé.

---

## 4. Fonctionnalités réalisées (socle)

- Génération de carte par bruit de Perlin, graine (*seed*) reproductible.
- 6 types de terrain avec bonus de ressources et coûts en PM distincts.
- Gestion des villes : croissance, famine, production, bâtiments, murailles.
- Unités : colon, guerrier ; combats automatiques contre unités et villes.
- Ressources globales : nourriture, production, science, or.
- Arbre technologique en graphe orienté (9 technologies, prérequis multiples).
- Barbares : campements, spawn progressif, pathfinding BFS, priorité des cibles.
- Conditions de victoire/défaite et calcul du score de fin de partie.

---

## 5. Extensions réalisées

| Extension | Description |
|---|---|
| **EXT-1** Navigation & Carte | Technologie **Bateaux** (traversée maritime, 2 PM), génération Perlin avec continents cohérents |
| **EXT-2** Merveilles & Ressources | 5 Merveilles du monde uniques, 5 ressources spéciales conditionnées par technologies |
| **EXT-4** Gestion de partie | Menu principal, sauvegarde/chargement, historique des scores |
| **EXT-5** Difficulté & Unités avancées | Presets Facile/Normal/Difficile, multiplicateurs configurables, unités **Archer** et **Cavalier** |
| **EXT-6** Brouillard de guerre | Statut par tuile : inconnue / explorée / visible, champ de vision unités et villes |

---

## 6. Conditions de victoire, défaite et score

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
score = (nb_technos × 100)
      + (nb_villes × 50)
      + (population_totale × 20)
      + max(0, (tours_max − tour_actuel) × 10)
```

---

## 7. Séquence d'un tour

Chaque tour s'exécute dans l'ordre suivant :

1. **Restauration des PM** — toutes les unités (joueur et barbares) récupèrent leur maximum.
2. **Revenus des villes** — chaque ville exploite ses tuiles et accumule nourriture, production, science, or.
3. **Croissance** — mise à jour de la population de chaque ville (croissance ou famine).
4. **Production** — avancement des projets (bâtiments, unités).
5. **Entretien** — le joueur paye l'entretien global en or ; en cas de découvert, une unité est dissoute.
6. **Recherche** — avancement de la technologie en cours.
7. **Phase barbares** — spawn depuis les campements puis déplacement et attaques.
8. **Fin de tour** — incrémentation du compteur, vérification des conditions de victoire/défaite.

---

## 8. Contrôles — mode CLI (ncurses)

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

## 9. Contrôles — mode SDL

| Touche / Action | Effet |
|---|---|
| Clic | Sélectionner une unité ou une ville |
| `z` `q` `s` `d` | Déplacer l'unité sélectionnée |
| Flèches directionnelles | Déplacer la caméra |
| `v` | Ouvrir le menu de ville |
| `t` | Ouvrir l'arbre des technologies |
| `n` | Terminer le tour |
| `Maj+S` | Sauvegarde manuelle |
| `Échap` | Retour au menu (sauvegarde auto dans `current_game.tmp`) |
| `F11` | Plein écran |

---

## 10. Arbre technologique

| Technologie | Coût | Prérequis | Effet principal |
|---|:---:|---|---|
| Chasse | 50 | — | +1 nourriture sur les tuiles Forêt |
| Agriculture | 60 | — | +10 % nourriture globale |
| Artisanat | 70 | — | Débloque Guerrier, +10 % production |
| Écriture | 80 | — | Débloque Bibliothèque, +10 % science |
| Équitation | 100 | — | Débloque Cavalier, +1 PM sur les unités |
| Irrigation | 90 | Agriculture | +20 % nourriture supplémentaire |
| Maçonnerie | 100 | Artisanat | Débloque Muraille (double les PV de la ville) |
| Commerce | 90 | Artisanat + Écriture | Débloque Marché, +10 % or |
| Bateaux | 110 | Commerce | Navigation maritime (coût 2 PM/tuile eau) |

```
Chasse       Agriculture    Artisanat    Écriture    Équitation
                  |          |    \          |
              Irrigation  Maçon. Commerce ←←←←←←←←←
                               └──────────────→ Bateaux
```

---

## 11. Ressources spéciales (EXT-2)

Environ 15 % des tuiles éligibles portent une ressource spéciale, productive uniquement si la technologie correspondante est débloquée.

| Ressource | Terrain | Technologie requise | Bonus |
|---|---|---|---|
| Blé | Plaine | Agriculture | +3 nourriture/tour |
| Fer | Montagne | Artisanat | +3 production/tour |
| Épices | Forêt, Désert | Commerce | +4 or/tour |
| Spécimens | Toundra | Écriture | +3 science/tour |
| Chevaux | Plaine, Toundra | Équitation | +1 PM sur les unités produites dans la ville |

---

## 12. Merveilles du monde (EXT-2)

Chaque Merveille est unique dans la partie. Elle se construit dans une ville et son effet est global une fois achevée.

| Merveille | Terrain requis | Coût | Effet |
|---|---|:---:|---|
| Pyramides | Désert | 200 | +50 % nourriture dans toutes les villes |
| Grande Bibliothèque | Toundra | 250 | Débloque immédiatement la technologie en cours |
| Colisée | Plaine | 200 | +2 or/tour par ville |
| Tour de Babel | Montagne | 300 | −25 % coût de toutes les futures technologies |
| Jardins suspendus | Forêt | 220 | −20 % seuil de croissance de toutes les villes |

---

## 13. Architecture

```
include/core/      En-têtes publics (types.h, game.h, map.h, …)
src/core/          Moteur de jeu (game.c, map.c, barbares.c, ressources.c, …)
                   Interface CLI (main.c / ncurses)
                   Interface SDL (engine_sdl.c)
tests/             Tests unitaires (test_barbares.c, test_physique.c, …)
docs/              Documents de gestion de projet
assets/            Police et images pour le mode SDL
```

Le moteur de jeu est **strictement découplé** de l'affichage : CLI et SDL partagent exactement les mêmes règles et structures.

---

## 14. Dépendances

| Dépendance | Usage |
|---|---|
| GCC (C11), make, pkg-config | Compilation |
| ncursesw | Mode CLI |
| SDL2, SDL2_ttf, SDL2_image | Mode SDL uniquement |

Installation des dépendances système :

```bash
make req
```

---

## 15. Compilation et lancement

```bash
make           # Compile le projet (mode CLI par défaut)
make cli       # Compile et lance en mode CLI
make sdl       # Compile avec SDL2 et lance en mode graphique
make test      # Compile et exécute les tests unitaires
make clean     # Supprime les fichiers de compilation et les exécutables
make reset     # Supprime également les données (sauvegardes, scores, config)
```

Exécutables produits dans `bin/civ`.

---

## 16. Paramètres de lancement

| Option longue | Forme courte | Défaut | Description |
|---|:---:|:---:|---|
| `--help` | — | — | Affiche l'aide et quitte |
| `--mode MODE` | `-m` | `cli` | Mode d'affichage : `cli` ou `sdl` |
| `--difficulty DIFF` | `-d` | `normal` | Preset : `facile`, `normal`, `difficile` |
| `--seed N` | `-s` | aléatoire | Graine de génération de la carte |
| `--width W` | `-W` | 50 | Largeur de la carte en tuiles |
| `--height H` | `-H` | 30 | Hauteur de la carte en tuiles |
| `--turns N` | `-t` | 200 | Nombre de tours maximum |
| `--barbarians N` | `-b` | 3 | Nombre de campements barbares initiaux |
| `--production-multiplier X` | `-p` | 1.0 | Multiplicateur de production |
| `--growth-multiplier X` | `-g` | 1.0 | Multiplicateur du seuil de croissance |
| `--barbarian-spawn-multiplier X` | `-B` | 1.0 | Multiplicateur de spawn barbare |
| `--famine on/off` | `-f` | `on` | Active ou désactive la famine |

Exemples :

```bash
./bin/civ --mode sdl --width 40 --height 25 --seed 12345
./bin/civ -m cli -t 150 -b 5 -d difficile
```

---

## 17. Presets de difficulté (EXT-5)

| Paramètre | Facile | Normal | Difficile |
|---|:---:|:---:|:---:|
| Multiplicateur de production | 1.25 | 1.0 | 0.75 |
| Multiplicateur seuil de croissance | 0.75 | 1.0 | 1.25 |
| Multiplicateur spawn barbare | 0.75 | 1.0 | 1.25 |
| Famine | désactivée | activée | activée |

Voir [docs/CONFIG.md](docs/CONFIG.md) pour les détails complets.

---

## 18. Tests unitaires

```bash
make test
```

| Fichier | Ce qui est testé |
|---|---|
| [tests/test_barbares.c](tests/test_barbares.c) | Init camps, spawn, déplacement, pathfinding, combat, destruction, intégration |
| [tests/test_physique.c](tests/test_physique.c) | Formules de combat, déplacement, PM |
| [tests/test_archer.c](tests/test_archer.c) | Statistiques et contraintes de l'Archer |
| [tests/test_cavalier.c](tests/test_cavalier.c) | Statistiques et contraintes du Cavalier |
| [tests/test_config.c](tests/test_config.c) | Lecture et application des presets de difficulté |
| [tests/test_difficulty_effects.c](tests/test_difficulty_effects.c) | Effets des multiplicateurs sur les règles du jeu |

---

## 19. Sauvegardes et scores

| Fichier | Rôle |
|---|---|
| `savegame.sav` | Sauvegarde manuelle (`Maj+S`) |
| `current_game.tmp` | Sauvegarde automatique de session (écrasée à chaque tour) |
| `score_history.txt` | Historique des scores (mode CLI) |
| `scores.txt` | Historique des scores (mode SDL) |

---

## 20. Documentation et gestion de projet

- Charte de projet : [docs/charte_projet_incivilises-1.pdf](docs/charte_projet_incivilises-1.pdf)
- Gantt final : [docs/gantt_civilization_final.pdf](docs/gantt_civilization_final.pdf)
- Presets de difficulté : [docs/CONFIG.md](docs/CONFIG.md)
- Comptes rendus de réunion : [docs/](docs/)

---

## 21. Limites connues

- Le mode SDL requiert une compilation spécifique avec SDL2 (`make sdl`) ; la cible `make` seule produit uniquement le binaire CLI.
- L'historique des scores est stocké dans deux fichiers distincts selon le mode : `score_history.txt` (CLI) et `scores.txt` (SDL).
