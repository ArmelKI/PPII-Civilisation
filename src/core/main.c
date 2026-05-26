#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/barbares.h"
#include "core/types.h"

#define MAP_W 20
#define MAP_H 12
#define MAX_TURNS 20
#define MAX_PLAYER_UNITS 8
#define MAX_BARBARIANS 16
#define NUM_BARBARIAN_CAMPS 3

// Je regroupe ici tout ce qui doit vivre pendant la partie.
// Ça évite de passer 15 paramètres dans chaque fonction.
typedef struct GameState {
	Map map;
	Tile *tile_buffer;

	Player player;
	City city;

	Unit *player_units[MAX_PLAYER_UNITS];
	int player_unit_count;

	Unit *barbarians[MAX_BARBARIANS];
	int barbarian_count;
	BarbarianCamp *camps;
	int camp_count;

	int turn;
	int game_over;
} GameState;

// Fonctions externes optionnelles.
// Si elles existent dans d'autres fichiers, elles prennent la main sans rien casser ici.
void city_income_wassim(Player *player, City *city) __attribute__((weak));
void display_state_maxence(const Map *map, const Player *player, const City *city,
	int turn, int player_units, int barbarians) __attribute__((weak));

static void print_help(void)
{
	puts("Usage: ./bin/civ [options]");
	puts("Options:");
	puts("  --help                 Affiche cette aide");
	puts("  --mode MODE | -m MODE  MODE = cli ou sdl");
}

static void init_map(GameState *game)
{
	// J'utilise un buffer continu pour les tuiles + un tableau de pointeurs de lignes.
	// Avantage: accès map.tiles[y][x] simple, sans allocation ligne par ligne.
	game->map.width = MAP_W;
	game->map.height = MAP_H;
	game->tile_buffer = (Tile *)calloc((size_t)MAP_W * (size_t)MAP_H, sizeof(Tile));
	game->map.tiles = (Tile **)malloc((size_t)MAP_H * sizeof(Tile *));

	if (!game->tile_buffer || !game->map.tiles) {
		fprintf(stderr, "Erreur d'allocation de la carte\n");
		exit(1);
	}

	for (int y = 0; y < MAP_H; ++y) {
		game->map.tiles[y] = &game->tile_buffer[y * MAP_W];
		for (int x = 0; x < MAP_W; ++x) {
			game->map.tiles[y][x].type = TERRAIN_PLAINE;
			game->map.tiles[y][x].city = NULL;
			game->map.tiles[y][x].unit = NULL;
		}
	}

	// Bordures en eau juste pour les tests:
	// ça force des cas où les déplacements doivent être refusés.
	for (int x = 0; x < MAP_W; ++x) {
		game->map.tiles[0][x].type = TERRAIN_EAU;
		game->map.tiles[MAP_H - 1][x].type = TERRAIN_EAU;
	}
}

static void init_player_and_city(GameState *game)
{
	// Valeurs de départ de test pour faire tourner le moteur sans attendre les autres modules.
	game->player.gold = 10;
	game->player.science = 0;
	game->player.camps_destroyed = 0;

	game->city.id = 1;
	game->city.population = 2;
	game->city.food_stored = 0;
	game->city.production_stored = 0;
	game->city.hp = 20;
	game->city.has_wall = 0;
	game->city.x = MAP_W / 2;
	game->city.y = MAP_H / 2;
	game->city.owner = &game->player;

	game->map.tiles[game->city.y][game->city.x].city = &game->city;

	Unit *warrior = (Unit *)malloc(sizeof(Unit));
	if (!warrior) {
		fprintf(stderr, "Erreur d'allocation d'unité\n");
		exit(1);
	}

	warrior->type = UNIT_WARRIOR;
	warrior->id = 2;
	warrior->hp = 15;
	warrior->hp_max = 15;
	warrior->attack = 3;
	warrior->defense = 2;
	warrior->pm = 3;
	warrior->pm_max = 3;
	warrior->x = game->city.x;
	warrior->y = game->city.y;
	warrior->owner = &game->player;

	game->player_units[0] = warrior;
	game->player_unit_count = 1;
	game->map.tiles[warrior->y][warrior->x].unit = warrior;
}

static void restore_pm(Unit **units, int count)
{
	// Début de tour : toutes les unités vivantes récupèrent leurs PM max.
	for (int i = 0; i < count; ++i) {
		if (!units[i] || units[i]->hp <= 0) {
			continue;
		}
		units[i]->pm = units[i]->pm_max;
	}
}

// Revenus de ville : j'utilise le hook externe si dispo, sinon un fallback local.
static void apply_city_income_wassim(Player *player, City *city)
{
	if (city_income_wassim) {
		city_income_wassim(player, city);
		return;
	}

	if (!player || !city || city->hp <= 0) {
		return;
	}

	player->gold += city->population;
	player->science += 1;
}

// Affichage d'état : j'utilise le hook externe si dispo, sinon affichage console.
static void print_state_maxence_stub(const GameState *game)
{
	if (display_state_maxence) {
		display_state_maxence(&game->map, &game->player, &game->city,
			game->turn, game->player_unit_count, game->barbarian_count);
		return;
	}

	printf("\n=== TOUR %d ===\n", game->turn);
	printf("Ressources: or=%d science=%d\n", game->player.gold, game->player.science);
	printf("Ville: hp=%d pop=%d (%d,%d)\n", game->city.hp, game->city.population, game->city.x, game->city.y);
	printf("Unités joueur: %d | Barbares: %d | Camps détruits: %d\n",
		game->player_unit_count, game->barbarian_count, game->player.camps_destroyed);
}

static void init_barbarian_camps(GameState *game)
{
	game->camp_count = NUM_BARBARIAN_CAMPS;
	game->camps = barbares_init_camps(&game->map, game->camp_count, game->city.x, game->city.y);
	if (!game->camps) {
		fprintf(stderr, "Erreur d'initialisation des campements barbares\n");
		exit(1);
	}
}

static void resolve_camp_destruction(GameState *game)
{
	for (int c = 0; c < game->camp_count; ++c) {
		BarbarianCamp *camp = &game->camps[c];
		if (camp->destroyed) {
			continue;
		}
		for (int i = 0; i < game->player_unit_count; ++i) {
			Unit *u = game->player_units[i];
			if (!u || u->hp <= 0 || u->type != UNIT_WARRIOR) {
				continue;
			}
			if (u->x == camp->x && u->y == camp->y) {
				barbares_destroy_camp(camp, &game->player);
				break;
			}
		}
	}
}

static void spawn_barbarians_from_camps(GameState *game)
{
	const int max_alive = game->camp_count * 3;
	for (int c = 0; c < game->camp_count; ++c) {
		if (game->barbarian_count >= max_alive || game->barbarian_count >= MAX_BARBARIANS) {
			return;
		}
		BarbarianCamp *camp = &game->camps[c];
		if (barbares_spawn_unit(camp, &game->map, game->barbarians, &game->barbarian_count, max_alive) == 0) {
			continue;
		}
	}
}

static void compact_dead_barbarians(GameState *game)
{
	int write_idx = 0;
	for (int i = 0; i < game->barbarian_count; ++i) {
		Unit *b = game->barbarians[i];
		if (!b || b->hp <= 0) {
			free(b);
			continue;
		}
		game->barbarians[write_idx++] = b;
	}
	game->barbarian_count = write_idx;
}

static void play_barbarian_phase(GameState *game)
{
	City *cities[1] = { &game->city };
	resolve_camp_destruction(game);
	spawn_barbarians_from_camps(game);

	for (int i = 0; i < game->barbarian_count; ++i) {
		Unit *barb = game->barbarians[i];
		if (!barb || barb->hp <= 0) {
			continue;
		}
		barbares_move(barb, &game->map, cities, 1, game->player_units, game->player_unit_count);
		(void)barbares_attack_if_adjacent(barb, &game->map, cities, 1, game->player_units, game->player_unit_count);
	}

	compact_dead_barbarians(game);
}

static void next_turn(GameState *game)
{
	// 1) Restauration PM
	restore_pm(game->player_units, game->player_unit_count);
	restore_pm(game->barbarians, game->barbarian_count);

	// 2) Revenus de ville
	apply_city_income_wassim(&game->player, &game->city);

	// 3) Phase Barbares
	play_barbarian_phase(game);
}

static void cleanup_game(GameState *game)
{
	for (int i = 0; i < game->player_unit_count; ++i) {
		free(game->player_units[i]);
	}

	for (int i = 0; i < game->barbarian_count; ++i) {
		free(game->barbarians[i]);
	}
	barbares_cleanup(game->camps, game->camp_count);

	free(game->map.tiles);
	free(game->tile_buffer);
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

	if (strcmp(mode, "sdl") == 0) {
		puts("[CIV] Mode SDL initialise (squelette).\nInstallez SDL2 et branchez l'UI plus tard.");
		return 0;
	}

	if (strcmp(mode, "cli") != 0) {
		fprintf(stderr, "Mode invalide: %s\n", mode);
		print_help();
		return 1;
	}

	srand((unsigned int)time(NULL));

	GameState game;
	memset(&game, 0, sizeof(GameState));
	init_map(&game);
	init_player_and_city(&game);
	init_barbarian_camps(&game);

	puts("[CIV] Boucle principale lancée (prototype).\n");

	// Boucle centrale demandée dans le ticket.
	while (!game.game_over) {
		print_state_maxence_stub(&game);
		next_turn(&game);

		game.turn++;
		if (game.turn >= MAX_TURNS || game.city.hp <= 0) {
			game.game_over = 1;
		}
	}

	puts("\nFin de partie (prototype).\n");
	cleanup_game(&game);
	return 0;
}
