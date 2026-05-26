# PPII - Early Civilization (Les Incivilises)

Projet PPII FISE 2025-2026. Sujet choisi: Civilization (version minimaliste).

[[_TOC_]]

---

## 1. Contrat academique (exigences README)

Ce README contient toutes les informations requises par le sujet:
- membres et roles;
- sujet choisi + extensions realisees;
- dependances logicielles;
- commandes de compilation, lancement et tests;
- parametres de lancement supportes;
- limites connues;
- liens vers les documents de gestion de projet (gantt, comptes rendus, charte).

## 2. Equipe

| Membre | Role principal | Perimetre technique |
| --- | --- | --- |
| Maxence ARDITTI | Lead technique | Architecture, carte, CLI, SDL, parser |
| Armel Stephane Novak KI | IA & barbares | Makefile, barbares, IA, extension |
| Ababacar DIENE | Unites, combat & technologie | Unites, combat, arbre technologique |
| Wassim OUKAZI | Villes, ressources & documentation | Villes, ressources, batiments, evenements |

## 3. Sujet et objectif

Early Civilization: jeu de strategie au tour par tour en C, jouable en CLI (ncurses) et en SDL2.
Le joueur developpe sa civilisation (villes, unites, technologies) et affronte les barbares.

## 4. Fonctionnalites principales (socle)

- Generation de carte par bruit de Perlin, seed reproductible.
- Gestion des villes, croissance, production, batiments, murs.
- Unites: colon, guerrier; combats contre unites et villes.
- Ressources globales: nourriture, production, science, or.
- Arbre technologique et recherche active.
- Barbares: camps, spawn progressif, attaques sur unites/villes.
- Conditions de victoire/defaite et score de fin.

## 5. Extensions et ajouts

- Brouillard de guerre (zones cachees/explorees/visibles).
- Unites avancees: archer, cavalier.
- Technologie bateau: deplacement sur l'eau.
- Ressources speciales conditionnees par technologies (ble, fer, epices, specimens, chevaux).
- Merveilles du monde (Pyramides, Grande Bibliotheque, Colisee, Tour de Babel, Jardins suspendus).
- Presets de difficulte + multiplicateurs configurables.
- Sauvegarde/chargement + historique des scores.

## 6. Regles de victoire, defaite et score

Victoire:
- Scientifique: toutes les technologies debloquees.
- Territoriale: 10 villes ou plus pendant 5 tours consecutifs.

Defaite:
- Plus aucune ville active.
- Limite de tours atteinte (200).
- Production nulle pendant 5 tours consecutifs.

Score final:
```
score = (nb_technos * 100) + (nb_villes * 50) + (population_totale * 20) + bonus_tours
bonus_tours = max(0, (200 - tour_actuel) * 10)
```

## 7. Sequence d'un tour

1. Restauration des PM (joueur + barbares).
2. Revenus des villes, croissance et production.
3. Paiement de l'entretien global (or).
4. Phase barbares: spawn + deplacement/attaques.
5. Mise a jour de la recherche.
6. Incrementation du compteur de tours.

## 8. Controles (CLI)

- Clic: selectionner une unite ou une ville.
- z/q/s/d: deplacer l'unite selectionnee.
- f: fonder une ville (colon).
- c / v: gerer la ville selectionnee (construction/recrutement).
- t: ouvrir l'arbre des technologies.
- n: terminer le tour.
- Maj+S: sauvegarder la partie.
- x: quitter le jeu.

La liste complete et a jour est accessible via le menu "Regles & commandes".

## 9. Controles (SDL)

- Clic: selectionner une unite ou une ville.
- z/q/s/d: deplacer l'unite selectionnee.
- Fleches: deplacer la camera.
- v: ouvrir le menu de ville.
- t: ouvrir l'arbre des technologies.
- n: terminer le tour.
- Maj+S: sauvegarde manuelle.
- Echap: retour au menu principal (sauvegarde auto sur current_game.tmp).
- F11: plein ecran fenetre.

## 10. Arbre technologique

| Techno | Cout | Prerequis | Effet principal |
| --- | --- | --- | --- |
| Chasse | 50 | - | +1 nourriture sur forets |
| Agriculture | 60 | - | +10% nourriture globale |
| Artisanat | 70 | - | Debloque guerrier/archer, +10% production |
| Ecriture | 80 | - | Debloque bibliotheque, +10% science |
| Equitation | 100 | - | Debloque cavalier, +1 PM |
| Irrigation | 90 | Agriculture | +20% nourriture supplementaire |
| Maconnerie | 100 | Artisanat | Debloque muraille (double PV ville) |
| Commerce | 90 | Artisanat | Debloque marche, +10% or |
| Bateau | 110 | Commerce | Traverser l'eau (cout: 2 PM) |

## 11. Ressources speciales

Environ 15% des tuiles eligibles contiennent une ressource speciale, visible
et productive uniquement si la technologie requise est debloquee.

| Ressource | Terrain(s) | Techno requise | Bonus |
| --- | --- | --- | --- |
| Ble | Plaine | Agriculture | +3 nourriture |
| Fer | Montagne | Artisanat | +3 production |
| Epices | Foret, desert | Commerce | +4 or |
| Specimens | Toundra | Ecriture | +3 science |
| Chevaux | Plaine, toundra | Equitation | +1 PM sur les unites produites |

## 12. Merveilles du monde

| Merveille | Terrain requis | Cout | Effet |
| --- | --- | --- | --- |
| Pyramides | Desert | 200 | +50% nourriture globale |
| Grande Bibliotheque | Toundra | 250 | Debloque immediatement la techno en cours |
| Colisee | Plaine | 200 | +2 or/tour par ville |
| Tour de Babel | Montagne | 300 | -25% cout des futures technos |
| Jardins suspendus | Foret | 220 | -20% seuil de croissance |

## 13. Architecture (vue rapide)

- Core moteur de jeu: [src/core](src/core)
- En-tetes publics: [include/core](include/core)
- UI CLI ncurses et UI SDL2 partageant la meme logique
- Tests unitaires: [tests](tests)

## 14. Dependances

- GCC (C11), make, pkg-config
- ncursesw
- SDL2, SDL2_ttf, SDL2_image (uniquement pour le mode SDL)

## 15. Compilation et execution

```bash
make
make cli
make sdl
make test
make clean
make reset
make req
```

Executables:
- CLI: ./bin/civ --mode cli
- SDL: ./bin/civ --mode sdl (compiler avec `make sdl`)

## 16. Parametres de lancement

Executable attendu: `civ`.

| Option | Valeur | Defaut | Description |
| --- | --- | --- | --- |
| `--help` | - | - | Affiche l'aide |
| `--mode` / `-m` | `cli` ou `sdl` | `cli` | Mode d'affichage |
| `--difficulty` / `-d` | `facile`, `normal`, `difficile` | `normal` | Preset de difficulte |
| `--production-multiplier` / `-p` | float | 1.0 | Multiplie la production |
| `--growth-multiplier` / `-g` | float | 1.0 | Multiplie le seuil de croissance |
| `--barbarian-spawn-multiplier` / `-B` | float | 1.0 | Multiplie le spawn barbare |
| `--famine` / `-f` | on/off, true/false, 1/0 | true | Active/desactive la famine |
| `--seed` / `-s` | int | aleatoire | Graine de generation |
| `--width` / `-W` | int | 50 | Largeur de la carte |
| `--height` / `-H` | int | 30 | Hauteur de la carte |
| `--turns` / `-t` | int | 200 | Tours max |
| `--barbarians` / `-b` | int | 3 | Nombre de campements initiaux |

Exemples:
```bash
./bin/civ --mode sdl --width 40 --height 25 --seed 12345
./bin/civ -m cli -t 150 -b 5
```

## 17. Presets de difficulte

Voir [docs/CONFIG.md](docs/CONFIG.md) pour les multiplicateurs exacts.

- Facile: production 1.25, croissance 0.75, spawn barbares 0.75, famine desactivee.
- Normal: production 1.0, croissance 1.0, spawn barbares 1.0, famine activee.
- Difficile: production 0.75, croissance 1.25, spawn barbares 1.25, famine activee.

## 18. Tests

Les tests unitaires couvrent le coeur du moteur:
- [tests/test_archer.c](tests/test_archer.c)
- [tests/test_barbares.c](tests/test_barbares.c)
- [tests/test_cavalier.c](tests/test_cavalier.c)
- [tests/test_config.c](tests/test_config.c)
- [tests/test_difficulty_effects.c](tests/test_difficulty_effects.c)
- [tests/test_physique.c](tests/test_physique.c)

Lancer avec:

```bash
make test
```

## 19. Sauvegardes et scores

- Sauvegarde manuelle: `savegame.sav`
- Sauvegarde auto de session: `current_game.tmp`
- Historique des scores: `score_history.txt`

## 20. Documentation et gestion de projet

- Charte de projet: [docs/charte_projet_incivilises-1.pdf](docs/charte_projet_incivilises-1.pdf)
- Gantt final: [docs/gantt_civilization_final.pdf](docs/gantt_civilization_final.pdf)
- Presets de difficulte: [docs/CONFIG.md](docs/CONFIG.md)
- Comptes rendus de reunion: [docs](docs)

## 21. Limites connues

- L'historique des scores est affiche en CLI a partir de `score_history.txt`; l'ecran SDL lit `scores.txt`.
- Le parametre `--turns` n'influence pas la condition de fin (limite fixe a 200 tours).
- Le mode SDL requiert une compilation specifique (`make sdl`).
