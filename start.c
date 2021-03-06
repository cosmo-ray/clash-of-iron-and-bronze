/**        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                   Version 2, December 2004
 *
 * Copyright (C) 2020 Matthias Gatto <matthias.gatto@protonmail.com>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */

/**
 * small rpg made in a very C style (and not YIRL C) for fun
 */

#include <yirl/all.h>

#define slinger_path "./slinger.png"
#define hoplite_path "./hoplite.png"
#define pritestess_path "./pritestess.png"
#define legioness_path "./legioness.png"
#define legioness_pilumless_path "./legioness_pilumless.png"

#define slash_path "./slash.png"
#define shield_path "./shild.png"

#define MYCENAEAN_REVENANT_PATH "./mycenaean_revenant.png"
#define AXEMAN_PATH "./axeman.png"
#define SPEARMAN_PATH "./spearman.png"
#define SPEARMAN_TROWER_PATH "./spearman_trower.png"

#define CASE_H 90
#define CASE_W 90

#define MOVE_COL "rgba: 0 0 127 120"
#define ATK_COL "rgba: 127 0 0 120"
#define HEAL_COL "rgba: 70 127 10 120"

enum {
	GOOD_SIDE = 0,
	BAD_SIDE = 1
};

enum {
	SPE_ATTACK,
	SPE_HEAL
};

#define SCREEN_PHONE 0
#define SCREEN_DESKTOP 1
int screen_type;

struct special {
	int range;
	int type;
	int amunition;
	int power;
	const char *name;
	void (*callback)(struct df *s, struct unit *u);
};

struct lvl_bonus {
	int hp;
	int atk_bonus;
	int atk_bonus_freq;
};

struct unit_type {
	int base_life;
	int range;
	int atk_dice;
	int def;
	int side;
	const char *path;
	struct special *spe;
	struct lvl_bonus *lvl_bonus;
};

static int old_tl;

static struct unit_type mycenaean_revenant = {
	50, 1, 20, 4, BAD_SIDE, MYCENAEAN_REVENANT_PATH, NULL, NULL
};

static struct unit_type axeman = {
	13, 1, 10, 0, BAD_SIDE, AXEMAN_PATH, NULL, NULL
};

static struct unit_type spearman = {
	10, 1, 5, 0, BAD_SIDE, SPEARMAN_PATH, NULL, NULL
};

static struct unit_type spearman_trower = {
	10, 2, 5, 0, BAD_SIDE, SPEARMAN_TROWER_PATH, NULL, NULL
};

static struct unit_type slinger = {
	10, 5, 4, 1, GOOD_SIDE, slinger_path, NULL, &(struct lvl_bonus){2, 1, 1}
};

static struct unit_type hoplite = {
	15, 1, 10, 3, GOOD_SIDE, hoplite_path, NULL, &(struct lvl_bonus){3, 2, 1}
};

static struct unit_type pritestess  = {
	10, 1, 4, 0, GOOD_SIDE, pritestess_path,
	&(struct special){2, SPE_HEAL, 4, 3, "heal", NULL},
	&(struct lvl_bonus){2, 1, 1}
};

static const char *mod_path;

static void legioness_rm_pilum(struct df *, struct unit *);
static struct unit_type legioness = {
	12, 1, 7, 3, GOOD_SIDE, legioness_path,
	&(struct special){2, SPE_ATTACK, 1, 9, "pilum", legioness_rm_pilum},
	&(struct lvl_bonus){3, 1, 1}
};


static int button_idx(int cx, int cy)
{
	if (screen_type == SCREEN_PHONE)
		return cx;
	return cy;
}

static is_on_button(int cx, int cy)
{
	if (screen_type == SCREEN_PHONE)
		return cy == 6;
	return cx == 4;
}

static struct unit_type *str_unit_type(const char *s)
{
	if (!strcmp(s, "mycenaean_revenant"))
		return &mycenaean_revenant;
	if (!strcmp(s, "axeman"))
		return &axeman;
	if (!strcmp(s, "spearman"))
		return &spearman;
	if (!strcmp(s, "spearman_trower"))
		return &spearman_trower;
	return NULL;
}

static const char *unit_type_str(struct unit_type *t)
{
	if (t == &pritestess)
		return "seer";
	else if (t == &legioness)
		return "legioness";
	else if (t == &hoplite)
		return "hoplite";
	else if (t == &slinger)
		return "slinger";
	return NULL;
}

struct unit {
	struct unit_type *t;
	int life;
	int max_life;
	int atk_bonus;
	int x;
	int y;
	int side;
	Entity *u;
	int has_move;
	int has_atk;
	struct special spe;
};

enum {
	NO_BUTTON,
	BUTTON_UNPUSH,
	BUTTON_PUSH
};

struct button {
	int state;
	Entity *rect;
	Entity *txt;
	void (*callback_push)(struct df *);
};

struct df {
	struct unit p0[4];
	struct unit p1[12];
	struct unit *map[6][4];
	Entity *other_square[6][4];
	Entity *e;
	Entity *select_square;
	Entity *mouse_over;
	Entity *lifes[6 * 4];
	int selected_x;
	int selected_y;
	int is_spe_mode;
	int turn_state;
	struct button bottom_buttom[4];
};

#define DF_MAP_FOR(idx_x, idx_y)			\
	for (int idx_y = 0; idx_y < 6; ++idx_y)		\
		for (int idx_x = 0; idx_x < 4; ++idx_x)

static void handle_special(struct df *);
static void rm_button(struct df *, int);
static void init_buttom(struct df *, int , const char *str, void (*)(struct df *));
static void end_turn(struct df *s);

static void wait_update(int us)
{
	ygUpdateScreen();
	usleep(us);
}

static void fill_square(struct df *, int, const char *,
			_Bool (*)(struct df *, struct unit *, struct unit *), int);

_Bool is_empty(struct df *s, struct unit *c, struct unit *o)
{
	return o == NULL;
}

_Bool is_enemy(struct df *s, struct unit *c, struct unit *o)
{
	return o && c && c->side != o->side;
}

_Bool is_ally(struct df *s, struct unit *c, struct unit *o)
{
	return o && c && c->side == o->side;
}

static Entity *mk_case_rect(Entity *e, int x, int y, const char *col, Entity *extra)
{
	Entity *r = ywCanvasNewRectangle(e, x * CASE_W, y * CASE_H, CASE_W, CASE_H, col);

	if (extra)
		yePushBack(r, extra, "extra");
	return r;
}

static void remove_unit(struct df *s, struct unit *u, int x, int y)
{
	printf("remove %d %d\n", x, y);
	ywCanvasRemoveObj(s->e, u->u);
	s->map[y][x] = NULL;
}

static void legioness_rm_pilum(struct df *s, struct unit *u)
{
	ywCanvasRemoveObj(s->e, u->u);
	u->u = ywCanvasNewImgByPath(s->e, u->x * CASE_W, u->y * CASE_H,
				    legioness_pilumless_path);
	ywCanvasForceSizeXY(u->u, CASE_W, CASE_H);
}

/**
 * @return who win, 0 if no winner
 */
static int print_lifes(struct df *s)
{
	int nb_units[2] = {0};

	/* remove all old lifes */
	for (int i = 0; s->lifes[i]; ++i) {
		ywCanvasRemoveObj(s->e, s->lifes[i]);
		s->lifes[i] = NULL;
	}

	int ll = 0;

	DF_MAP_FOR(ix, iy) {
		struct unit *u = s->map[iy][ix];

		if (u) {
			int cl = u->life * ((CASE_W - 5) / (double)u->max_life);

			s->lifes[ll++] =
				ywCanvasNewRectangle(s->e, ix * CASE_W, iy * CASE_H, cl, 6,
						     "rgba: 0 255 0 100");
			nb_units[u->side]++;
		}
	}
	if (!nb_units[0])
		return 2;
	if (!nb_units[1])
		return 1;
	return 0;
}

static void move_to(struct df *s, struct unit *su, int cx, int cy)
{
	int sx = su->x, sy = su->y;

	s->map[cy][cx] = su;
	s->map[sy][sx] = NULL;
	ywCanvasObjSetPos(su->u, cx * CASE_W, cy * CASE_H);
	su->x = cx;
	su->y = cy;
	su->has_move = 1;
}

static _Bool can_select_unit(struct df *s, int x, int y)
{
	if (!s->map[y][x])
		return 0;

	struct unit *u = s->map[y][x];

	return u->side == s->turn_state;
}

static void empty_osquare(struct df *s)
{
	DF_MAP_FOR(ix, iy) {
		ywCanvasRemoveObj(s->e, s->other_square[iy][ix]);
		s->other_square[iy][ix] = NULL;
	}
}

static void unselect(struct df *s)
{
	ywCanvasRemoveObj(s->e, s->select_square);
	empty_osquare(s);
	s->select_square = NULL;
	s->is_spe_mode = 0;
	rm_button(s, 0);
}

static void atk_squares_fill(struct df *s, struct unit *u)
{
	empty_osquare(s);
	if (!u->has_move)
		fill_square(s, 1, MOVE_COL, is_empty, 0);
	if (!u->has_atk)
		fill_square(s, u->t->range, ATK_COL, is_enemy, 1);

}

static void normal_atk_mode(struct df *s)
{
	struct unit *u = s->map[s->selected_y][s->selected_x];

	s->is_spe_mode = 0;
	rm_button(s, 0);
	atk_squares_fill(s, u);
	init_buttom(s, 0, u->spe.name, handle_special);
}

static void handle_special(struct df *s)
{
	struct unit *u = s->map[s->selected_y][s->selected_x];

	s->is_spe_mode = 1;
	rm_button(s, 0);
	empty_osquare(s);
	if (u->spe.type == SPE_HEAL)
		fill_square(s, u->spe.range, HEAL_COL, is_ally, 2);
	else
		fill_square(s, u->spe.range, ATK_COL, is_enemy, 1);
	init_buttom(s, 0, "normal\nattack", normal_atk_mode);
	printf("handle special !\n");
}

static int find_attackable(struct df *s, int cx, int cy, int range, struct unit **enemies)
{
	int ret = 0;

	DF_MAP_FOR(ix, iy) {
		int diff = abs(cx - ix) + abs(cy - iy);

		if (diff && diff <= range && is_enemy(s, s->map[cy][cx], s->map[iy][ix])) {
			enemies[ret++] = s->map[iy][ix];
		}
	}
	return ret;
}

static void fill_square(struct df *s, int range, const char *col,
			_Bool (*check)(struct df *, struct unit *, struct unit *), int type)
{
	int cx = s->selected_x;
	int cy = s->selected_y;

	DF_MAP_FOR(ix, iy) {
		int diff = abs(cx - ix) + abs(cy - iy);
		if (diff && diff <= range && check(s, s->map[cy][cx], s->map[iy][ix])) {
			yeAutoFree Entity *extra = yeCreateInt(type, NULL, NULL);

			s->other_square[iy][ix] = mk_case_rect(s->e, ix, iy, col, extra);
		}
	}
}

void do_attack(struct df *s, struct unit *au,
	       struct unit *ou)
{
	int ax = au->x, ay = au->y, ox = ou->x, oy = ou->y;
	int dice = s->is_spe_mode == 1 ? au->spe.power : au->t->atk_dice;
	int roll = yuiRand() % dice + 1;
	int dmg = roll;

	while (roll == au->t->atk_dice) {
		printf("critical !\n");
		dmg += roll;
		roll = yuiRand() % au->t->atk_dice + 1;
	}
	dmg -= ou->t->def;
	dmg += au->atk_bonus;
	printf("do dmg %d, dice %d\n", dmg, dice);
	if (dmg > 0) {
		Entity *slash;

		ou->life -= dmg;
		if (ou->life < 0)
			remove_unit(s, ou, ox, oy);
		slash = ywCanvasNewImgByPath(s->e, ox * CASE_W, oy * CASE_H, slash_path);
		wait_update(100000);
		ywCanvasMoveObjXY(slash, 10, 10);
		wait_update(100000);
 		ywCanvasMoveObjXY(slash, -20, -20);
		wait_update(100000);
		ywCanvasRemoveObj(s->e, slash);
	} else {
		Entity *shield = ywCanvasNewImgByPath(s->e, ox * CASE_W, oy * CASE_H, shield_path);

		wait_update(300000);
		ywCanvasRemoveObj(s->e, shield);
	}
	au->has_atk = 1;
	if (s->is_spe_mode) {
		au->spe.amunition -= 1;
		if (au->spe.callback)
			au->spe.callback(s, au);
	}

}

#define TRY_ATTACK()							\
	if ((nb_enemies = find_attackable(s, x, y, u->t->range, enemies)) > 0) { \
		ou = enemies[yuiRand() % nb_enemies];			\
		printf("attack\n");					\
		do_attack(s, u, ou);					\
		continue;						\
	}

void *dungeon_fight_action(int nbArgs, void **args)
{
	Entity *df = args[0];
	Entity *eves = args[1];
	struct df *s = yeGetDataAt(df, "data");
	int winner;

	if ((winner = print_lifes(s)) != 0) {
		if (winner == 1 && yeGet(df, "win-action")) {
			printf("win %p\n", yeGet(df, "win-action"));
			printf("%s\n", yeToCStr(yeGet(df, "win-action"), -1, YE_FORMAT_PRETTY));
			yesCall(yeGet(df, "win-action"), df);
		} else {
			if (yeGet(df, "quit"))
				yesCall(yeGet(df, "quit"), df);
			else
				ygTerminate();
			printf("winner %d\n", winner);
		}
		return (void *)ACTION;
	}

	if (s->turn_state != 0) {
		printf("ai turn\n");
		/*
		 * enemy been barbaric, they don't have any notiong of strategic, or group thinking
		 * they just attack the neerest enemy
		 */
		DF_MAP_FOR(x, y) {
			struct unit *u = s->map[y][x];

			if (!u || !u->life || u->side != s->turn_state || (u->has_move && u->has_atk))
				continue;
			struct unit *enemies[4];
			int nb_enemies;
			struct unit *ou;
			struct unit *ne = NULL;
			int odiff = 100;

			TRY_ATTACK()
			DF_MAP_FOR(ox, oy) {
				int diff;
				ou = s->map[oy][ox];

				if (!ou || ou->side == s->turn_state)
					continue;
				diff = abs(x - ox) + abs(x - oy);
				if (diff < odiff) {
					ne = ou;
					odiff = diff;
				}
			}

			if (u->has_move)
				continue;
			if (ne) {
				int other_test = 0;

				if (abs(x - ne->x) > abs(y - ne->y)) {
				test_x:
					if (ne->x > x && !s->map[y][x + 1]) {
						move_to(s, u, x + 1, y);
					} else if (ne->x < x && !s->map[y][x - 1]) {
						move_to(s, u, x - 1, y);
					} else if (!other_test) {
						other_test = 1;
						goto test_y;
					}
				} else {
				test_y:
					if (ne->y > y && !s->map[y + 1][x]) {
						move_to(s, u, x, y + 1);
					} else if (ne->y < y && !s->map[y - 1][x]) {
						move_to(s, u, x, y - 1);
					} else if (!other_test) {
						other_test = 1;
						goto test_x;
					}
				}
			}
			TRY_ATTACK()
		}
		end_turn(s);
	}

	int cx = yeveMouseX() / CASE_W;
	int cy = yeveMouseY() / CASE_H;

	ywCanvasRemoveObj(s->e, s->mouse_over);
	s->mouse_over = NULL;

	if (yevCheckKeys(eves, YKEY_MOUSEDOWN, 1)) {

		// buttom
		if (is_on_button(cx, cy)) {
			int idx = button_idx(cx, cy);
			if (s->bottom_buttom[idx].state == BUTTON_UNPUSH) {
				s->bottom_buttom[idx].callback_push(s);
				s->mouse_over =
					mk_case_rect(s->e, cx, cy,
						     "rgba: 120 120 120 100",
						     NULL);
			}
			return NULL;
		}

		if (cx >= 4 || cy >= 6)
			return NULL;

		if (!s->select_square && can_select_unit(s, cx, cy)) {
			struct unit *u = s->map[cy][cx];

			s->select_square = mk_case_rect(df, cx, cy, "rgba: 127 127 127 120", NULL);
			s->selected_x = cx;
			s->selected_y = cy;

			atk_squares_fill(s, u);
			if (u->spe.amunition && !u->has_atk)
				init_buttom(s, 0, u->spe.name, handle_special);
		} else if (cx == s->selected_x && cy == s->selected_y) {
			unselect(s);
		} else if (s->other_square[cy][cx]) {
			int action = yeGetIntAt(s->other_square[cy][cx], "extra");
			int sx = s->selected_x, sy = s->selected_y;
			struct unit *su = s->map[sy][sx];

			if (action == 1) {
				printf("attack\n");

				struct unit *ou = s->map[cy][cx];
				do_attack(s, su, ou);
			} else if (action == 0) {
				move_to(s, su, cx, cy);
			} else if (action == 2) {
				struct unit *ou = s->map[cy][cx];

				if (ou && ou->life < ou->max_life) {
					ou->life += yuiRand() % su->spe.power + 1 + su->atk_bonus;
					su->spe.amunition -= 1;
					if (ou->life > ou->max_life)
						ou->life = ou->max_life;
					if (su->spe.callback)
						su->spe.callback(s, su);
				}
			}
			unselect(s);
		}
		printf("mouse down %d %d\n", cx, cy);
	}
	if (yevCheckKeys(eves, YKEY_MOUSEUP, 1)) {
		printf("mouse up %d %d\n", yeveMouseX() / CASE_W, yeveMouseY() / CASE_H);
	}
	return NULL;
}

static  void init_unit(struct unit *u, struct df *df, struct unit_type *t, int x, int y, int side)
{
	int lvl = yeGetIntAt(yeGet(df->e, "lvls"), unit_type_str(t));

	u->u = ywCanvasNewImgByPath(df->e, x * CASE_W, y * CASE_H, t->path);
	ywCanvasForceSizeXY(u->u, CASE_W, CASE_H);
	u->t = t;
	u->max_life = t->base_life;
	u->x = x;
	u->y = y;
	u->atk_bonus = 0;
	df->map[y][x] = u;
	u->side = side;
	if (t->spe)
		u->spe = *t->spe;
	if (lvl) {
		u->max_life += lvl * t->lvl_bonus->hp;
		u->atk_bonus += lvl * t->lvl_bonus->atk_bonus / t->lvl_bonus->atk_bonus_freq;
	}
	printf("init %s at %d %d, lvl %d\n", unit_type_str(t), u->max_life, u->atk_bonus, lvl);
	u->life = u->max_life;
}

static void end_turn(struct df *s)
{
	unselect(s);
	++s->turn_state;
	s->turn_state &= 1;
	DF_MAP_FOR(x, y) {
		struct unit *u = s->map[y][x];

		if (u) {
			u->has_move = 0;
			u->has_atk = 0;
		}
	}
}

static void rm_button(struct df *s, int idx)
{
	struct button *b = &s->bottom_buttom[idx];

	b->state = NO_BUTTON;
	ywCanvasRemoveObj(s->e, b->rect);
	ywCanvasRemoveObj(s->e, b->txt);
}

static void init_buttom(struct df *df, int idx, const char *str, void (*callback)(struct df *))
{
	struct button *b = &df->bottom_buttom[idx];
	if (b->state != NO_BUTTON)
		return;
	b->state = BUTTON_UNPUSH;
	int x, y;
	if (screen_type == SCREEN_PHONE) {
		x = idx * CASE_W;
		y =  6 * CASE_H;
	} else {
		x = 4 * CASE_W;
		y =  idx * CASE_H;
	}
	b->rect = ywCanvasNewRectangle(df->e, x, y + 6,
				       CASE_W, CASE_H, "rgba: 40 255 70 255");
	b->txt = ywCanvasNewTextByStr(df->e, x + 4,
				      y + CASE_H / 2, str);
	b->callback_push = callback;
}

void *dungeon_fight_kaboum(int nbArgs, void **args)
{
	ywSetTurnLengthOverwrite(old_tl);
	ygModDirOut();
	return NULL;
}

void *dungeon_fight_init(int nbArgs, void **args)
{
	Entity *df = args[0];

	ygModDir("dungeon-fight");
	YEntityBlock {
		df.background = "rgba: 255 255 255 255";
		df.action = dungeon_fight_action;
		df.destroy = dungeon_fight_kaboum;
	}

	void *ret = ywidNewWidget(df, "canvas");
	int ww = ywRectW(yeGet(df, "wid-pix"));
	int wh = ywRectH(yeGet(df, "wid-pix"));

	if (ww > wh)
		screen_type = SCREEN_DESKTOP;
	struct df *df_st = calloc(1, sizeof *df_st);
	Entity *df_data = yeCreateData(df_st, df, "data");
	yeAutoFree Entity *bg_size = ywSizeCreate(4 * CASE_W, 6 * CASE_H, NULL, NULL);
	Entity *excludes = yeGet(df, "exclude");
	int excluded = 0;
	Entity *enemies = yeGet(df, "enemies");

	df_st->e = df;

	ywCanvasForceSizeXY(ywCanvasNewImgByPath(df, 0, 0, "./road.png"), 4 * CASE_W, 6 * CASE_H);

	YE_FOREACH(excludes, e) {
		excluded |= (1 * !strcmp(yeGetString(e), "slinger")) << 0;
		excluded |= (1 * !strcmp(yeGetString(e), "hoplite")) << 1;
		excluded |= (1 * !strcmp(yeGetString(e), "seer")) << 2;
		excluded |= (1 * !strcmp(yeGetString(e), "legioness")) << 3;
	}

	if (!(excluded & 1 << 1))
		init_unit(&df_st->p0[0], df_st, &hoplite, 1, 2, GOOD_SIDE);
	if (!(excluded & 1))
		init_unit(&df_st->p0[1], df_st, &slinger, 0, 2, GOOD_SIDE);
	if (!(excluded & 1 << 2))
		init_unit(&df_st->p0[2], df_st, &pritestess, 0, 3, GOOD_SIDE);
	if (!(excluded & 1 << 3))
		init_unit(&df_st->p0[3], df_st, &legioness, 1, 3, GOOD_SIDE);

	for (int i = 0, j = 0; i < 12; ++i) {
		const char *enemy = yeGetStringAt(enemies, i);
		if (!enemy)
			continue;
		printf("%d %s: %d %d\n", i, enemy, 2 + (i & 1), i / 2);
		init_unit(&df_st->p1[j++], df_st, str_unit_type(enemy),
			  2 + (i & 1), i / 2, BAD_SIDE);
	}

        /* make grid */
	for (int i = 0; i < 5; ++i)
		ywCanvasNewVSegment(df, i * CASE_W, 0, CASE_H * 6,
				    "rgba: 0 0 0 155");
	for (int i = 0; i < 7; ++i)
		ywCanvasNewHSegment(df, 0, i * CASE_H, CASE_W * 4,
				    "rgba: 0 0 0 155");
	for (int i = 0; i < 4; ++i) {
		df_st->bottom_buttom[i].state = NO_BUTTON;
	}
	init_buttom(df_st, 3, "End\nTurn", end_turn);
	yeSetFreeAdDestroy(df_data);
	df_st->turn_state = 0;
	old_tl = ywGetTurnLengthOverwrite();
	ywSetTurnLengthOverwrite(100000);
	return ret;
}

#include "story.c"

void *mod_init(int nbArg, void **args)
{
  	Entity *mod = args[0];
	Entity *init;
	Entity *init2;

	init = yeCreateArray(NULL, NULL);
	YEntityBlock {
		init.name = "dungeon-fight";
		init.callback = dungeon_fight_init;
	}
	ywidAddSubType(init);

	init2 = yeCreateArray(NULL, NULL);
	YEntityBlock {
		init2.name = "dungeon-story";
		init2.callback = dungeon_story_init;
		mod.name = "dungeon-fight";
		mod.test_dungeon_story = [];
		mod.test_dungeon_story["<type>"] = "dungeon-story";
		mod.test_dungeon_story.file = "story.json";
		mod.starting_widget = "test_dungeon_story";
		//mod["window size"] = [400, 650];
		mod["window size"] = [800, 600];
		mod["window name"] = "test_dungeon_story";
	}
	ywidAddSubType(init2);

	return mod;
}
