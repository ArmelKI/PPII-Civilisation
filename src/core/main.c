#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "../../include/core/map.h"

/*
 * Point d'entree minimal du projet.
 * Ce fichier sert de base commune pour l'equipe, et sera enrichi au fil
 * du semestre (parsing complet, initialisation de la carte, boucle de jeu...).
 */

static void print_help(void)
{
	puts("Usage: ./bin/civ [options]");
	puts("Options:");
	puts("  --help                 Affiche cette aide");
	puts("  --mode MODE | -m MODE  MODE = cli ou sdl");
	puts("  --seed N    | -s N    Graine de génération de la carte (aléatoire par défaut)");
    puts("  --width W   | -W W    Largeur de la carte en tuiles (par défaut: 50)");
    puts("  --height H  | -H H    Hauteur de la carte en tuiles (par défaut: 30)");
	puts("  --turns N   | -t N    Nombre de tours maximum (par défaut: 200)");
    puts("  --barbarians N | -b N Nombre de campements barbares initiaux (par défaut: 3)");
}

int main(int argc, char **argv)
{
	const char *mode = "cli";
	int width = 50;
	int height = 30;
	int seed;
	bool is_seed = false;
	int nb_turns = 200;
	int nb_barbarians = 3;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0) {
			print_help();
			return 0;
		}

		if ((strcmp(argv[i], "--mode") == 0 || strcmp(argv[i], "-m") == 0) && i + 1 < argc) {
			mode = argv[i + 1];
			++i;
			continue;
		}

		if ((strcmp(argv[i], "--seed") == 0 || strcmp(argv[i], "-s") == 0) && i + 1 < argc) {
			seed = atoi(argv[i + 1]);
			is_seed = true;
			++i;
			continue;
		}

		if ((strcmp(argv[i], "--width") == 0 || strcmp(argv[i], "-W") == 0) && i + 1 < argc) {
			width = atoi(argv[i + 1]);
			++i;
			continue;
		}

		if ((strcmp(argv[i], "--height") == 0 || strcmp(argv[i], "-H") == 0) && i + 1 < argc) {
			height = atoi(argv[i + 1]);
			++i;
			continue;
		}

		if ((strcmp(argv[i], "--turns") == 0 || strcmp(argv[i], "-t") == 0) && i + 1 < argc) {
			nb_turns = atoi(argv[i + 1]);
			++i;
			continue;
		}

		if ((strcmp(argv[i], "--barbarians") == 0 || strcmp(argv[i], "-b") == 0) && i + 1 < argc) {
			nb_barbarians = atoi(argv[i + 1]);
			++i;
			continue;
		}

		fprintf(stderr, "Option invalide: %s\n", argv[i]);
		print_help();
		return 1;
	}

	if (!is_seed) {
		seed = (int)time(NULL);
	}
	srand(seed); // On utilise la seed générée ou donnée par l'utilisateur pour générer la carte

	// Initialisation du joueur

	Player* player = malloc(sizeof(Player));
	player->gold = 0;
	player->science = 0;

	printf("[CIV] Generation avec la graine : %d\n", seed);

// ----------------- AFFICHAGE CLI -----------------

	if (strcmp(mode, "cli") == 0) {
		puts("[CIV] Mode CLI initialise (squelette).");
		Map* Map = create_map(width, height, player);
		display_map(Map);
		display_map_color(Map);
		free_map(Map);
		free(player);
		return 0;
	}

// ----------------- AFFICHAGE SDL -----------------

	if (strcmp(mode, "sdl") == 0) {
		puts("[CIV] Mode SDL initialise (squelette).\nInstallez SDL2 et branchez l'UI plus tard.");
		free(player);
		return 0;
	}

	// mode invalide
    fprintf(stderr, "Mode invalide: %s\n", mode);
    print_help();
    free(player);
    return 1;
}
