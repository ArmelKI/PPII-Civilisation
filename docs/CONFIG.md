# Configuration des presets de difficulté

Ce fichier documente les valeurs par défaut utilisées dans le Socle de Difficulté.

Presets disponibles : Facile / Normal / Difficile

La difficulté se choisit au lancement avec `--difficulty` ou `-d` et accepte les valeurs `facile`, `normal` ou `difficile`.

Les surcharges détaillées sont également disponibles au lancement:
- `--production-multiplier` / `-p`
- `--growth-multiplier` / `-g`
- `--barbarian-spawn-multiplier` / `-B`
- `--famine` / `-f` avec `on`, `off`, `true`, `false`, `1`, `0`

- Facile (DIFF_EASY)
  - production_multiplier: 1.25
  - growth_threshold_multiplier: 0.75
  - barbarian_spawn_multiplier: 0.75
  - famine_enabled: false

- Normal (DIFF_NORMAL)
  - production_multiplier: 1.0
  - growth_threshold_multiplier: 1.0
  - barbarian_spawn_multiplier: 1.0
  - famine_enabled: true

- Difficile (DIFF_HARD)
  - production_multiplier: 0.75
  - growth_threshold_multiplier: 1.25
  - barbarian_spawn_multiplier: 1.25
  - famine_enabled: true

Remarques:
- Les multiplicateurs sont appliqués par le moteur de jeu aux valeurs standards (production, seuils, fréquence de spawn des barbares).
- Les paramètres peuvent être raffinés ou exposés en ligne de commande / fichier `config.txt` dans les livrables suivants.
