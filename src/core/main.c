#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/types.h"

#define MAP_W 20
#define MAP_H 12
#define MAX_TURNS 20
#define MAX_PLAYER_UNITS 8
#define MAX_BARBARIANS 16

typedef struct GameState {
	Map map;
	Tile *tile_buffer;

	Player player;
	City city;

	Unit *player_units[MAX_PLAYER_UNITS];
	int player_unit_count;

	Unit *barbarians[MAX_BARBARIANS];
	int barbarian_count;

	int turn;
	int game_over;
} GameState;

// Hooks optionnels pour brancher facilement le code des autres membres.
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
	game->map.width = MAP_W;
	game->map.height = MAP_H;
	game->tile_buffer = (Tile *)calloc((size_t)MAP_W * (size_t)MAP_H, sizeof(Tile));
	game->map.tiles = (Tile **)malloc((size_t)MAP_H * sizeof(Tile *));

	if (!game->tile_buffer || !game->map.tiles) {
		fprintf(stderr, "Erreur allocation map\n");
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

	for (int x = 0; x < MAP_W; ++x) {
		game->map.tiles[0][x].type = TERRAIN_EAU;
		game->map.tiles[MAP_H - 1][x].type = TERRAIN_EAU;
	}
}

static void init_player_and_city(GameState *game)
{
	game->player.gold = 10;
	game->player.science = 0;

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
		fprintf(stderr, "Erreur allocation unit\n");
		exit(1);
	}

	warrior->type = UNIT_WARRIOR;
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
	for (int i = 0; i < count; ++i) {
		if (!units[i] || units[i]->hp <= 0) {
			continue;
		}
		units[i]->pm = units[i]->pm_max;
	}
}

// Placeholder revenus ville (Wassim)
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

// Affichage temporaire de l'etat (en attendant l'affichage final de Maxence)
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
	printf("Unites joueur: %d | Barbares: %d\n", game->player_unit_count, game->barbarian_count);
}

static void spawn_random_barbarian(GameState *game)
{
	if (game->barbarian_count >= MAX_BARBARIANS) {
		return;
	}

	if ((rand() % 100) >= 35) {
		return;
	}

	int x = rand() % MAP_W;
	int y = rand() % MAP_H;

	if (game->map.tiles[y][x].type == TERRAIN_EAU) {
		return;
	}

	if (game->map.tiles[y][x].unit || game->map.tiles[y][x].city) {
		return;
	}

	Unit *barb = (Unit *)malloc(sizeof(Unit));
	if (!barb) {
		return;
	}

	barb->type = UNIT_BARBARIAN;
	barb->hp = 15;
	barb->hp_max = 15;
	barb->attack = 3;
	barb->defense = 2;
	barb->pm = 3;
	barb->pm_max = 3;
	barb->x = x;
	barb->y = y;
	barb->owner = NULL;

	game->barbarians[game->barbarian_count++] = barb;
	game->map.tiles[y][x].unit = barb;
}

static void move_barbarians_random(GameState *game)
{
	for (int i = 0; i < game->barbarian_count; ++i) {
		Unit *barb = game->barbarians[i];
		if (!barb || barb->hp <= 0) {
			continue;
		}

		int dx = (rand() % 3) - 1;
		int dy = (rand() % 3) - 1;
		if (dx == 0 && dy == 0) {
			continue;
		}

		int nx = barb->x + dx;
		int ny = barb->y + dy;

		if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) {
			continue;
		}

		if (game->map.tiles[ny][nx].type == TERRAIN_EAU) {
			continue;
		}

		if (game->map.tiles[ny][nx].unit || game->map.tiles[ny][nx].city) {
			continue;
		}

		game->map.tiles[barb->y][barb->x].unit = NULL;
		barb->x = nx;
		barb->y = ny;
		barb->pm -= 1;
		game->map.tiles[barb->y][barb->x].unit = barb;
	}
}

static void next_turn(GameState *game)
{
	restore_pm(game->player_units, game->player_unit_count);
	restore_pm(game->barbarians, game->barbarian_count);

	apply_city_income_wassim(&game->player, &game->city);

	spawn_random_barbarian(game);
	move_barbarians_random(game);
}

static void cleanup_game(GameState *game)
{
	for (int i = 0; i < game->player_unit_count; ++i) {
		free(game->player_units[i]);
	}

	for (int i = 0; i < game->barbarian_count; ++i) {
		free(game->barbarians[i]);
	}

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

	puts("[CIV] Boucle principale lancee (prototype).\n");

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
