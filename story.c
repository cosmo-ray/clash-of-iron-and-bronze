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

void *dungeon_story_init(int nbArgs, void **args)
{
	Entity *e = args[0];
	printf("dungeon_story_init %s\n", yeGetStringAt(e, "file"));
	yeAutoFree Entity *fe =  ygFileToEnt(YJSON, yeGetStringAt(e, "file"), NULL);
	Entity *next = yeCreateArray(e, "next");
	yeAutoFree Entity *lvls = yeCreateArray(NULL, NULL);

	YEntityBlock {
		e.background = "rgba: 255 255 255 255";
		e.action = "nextOnKeyDown";
		e.story = fe;
		e.story_idx = 0;
		lvls.slinger = 0;
		lvls.hoplite = 0;
		lvls.sear = 0;
		lvls.legioness = 5;
		next["<type>"] = "dungeon-fight";
	}

	Entity *cur_s = yeGet(fe, 0);
	yeGetPush2(cur_s, "pre-battle-txt", e, "text");
	yeGetPush(cur_s, next, "exclude");
	yeGetPush(cur_s, next, "enemies");
	yeCreateString(ygGet("callNext"), next, "win-action");
	yeCreateCopy(lvls, next, "lvls");

	Entity *last = yeCreateArray(next, "next");
	yeCreateString("text-screen", last, "<type>");
	yeGetPush2(cur_s, "win-txt", last, "text");
	
	void *ret = ywidNewWidget(e, "text-screen");
	printf("%s\n", yeToCStr(e, -1, YE_FORMAT_PRETTY));
	return ret;
}
