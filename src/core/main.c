#include <stdio.h>
#include <string.h>
#include "core/ressources.h"

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
}

int main(int argc, char **argv)
{
	const char *mode = "cli";

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

		fprintf(stderr, "Option invalide: %s\n", argv[i]);
		print_help();
		return 1;
	}

	if (strcmp(mode, "cli") == 0) {
		puts("[CIV] Mode CLI initialise (squelette).");
		return 0;
	}

	if (strcmp(mode, "sdl") == 0) {
		puts("[CIV] Mode SDL initialise (squelette).\nInstallez SDL2 et branchez l'UI plus tard.");
		return 0;
	}

	fprintf(stderr, "Mode invalide: %s\n", mode);
	print_help();
	return 1;
}
