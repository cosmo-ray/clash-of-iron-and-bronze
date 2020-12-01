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
	yeAutoFree Entity *lvls = yeCreateArray(NULL, NULL);

	YEntityBlock {
		e.background = "rgba: 255 255 255 255";
		e.action = "nextOnKeyDown";
		e.story = fe;
		e.story_idx = 0;
		lvls.slinger = 0;
		lvls.hoplite = 0;
		lvls.seer = 0;
		lvls.legioness = 0;
	}

	Entity *cur_s;
	Entity *tmpe = e;
	Entity *last;

	for (int i = 0; (cur_s = yeGet(fe, i)) != NULL; ++i) {
		if (i) {
			yeReCreateString("nextOnKeyDown", last, "action");
			tmpe = yeCreateArray(last, "next");			
			yeCreateString("text-screen", tmpe, "<type>");
			yeCreateString("rgba: 255 255 255 255", tmpe, "background");
			yeCreateString("nextOnKeyDown", tmpe, "action");
		}
		yeGetPush2(cur_s, "pre-battle-txt", tmpe, "text");

		Entity *next = yeCreateArray(tmpe, "next");
		yeCreateString("dungeon-fight", next, "<type>");
		yeGetPush(cur_s, next, "exclude");
		yeGetPush(cur_s, next, "enemies");
		printf("callNext: %p\n", ygGet("callNext"));
		yePushBack(next, ygGet("callNext"), "win-action");
		printf("%s\n", yeToCStr(lvls, -1, YE_FORMAT_PRETTY));
		yeCreateCopy(lvls, next, "lvls");
		YE_FOREACH(lvls, l) {
			yeAdd(l, 1);
		}
		YE_FOREACH(yeGet(cur_s, "exclude"), ex) {
			yeAddAt(lvls, yeGetString(ex), -1);
		}

		last = yeCreateArray(next, "next");
		yeCreateString("text-screen", last, "<type>");
		yeCreateString("rgba: 255 255 255 255", last, "background");
		yeGetPush2(cur_s, "win-txt", last, "text");
		yeCreateString("QuitOnKeyDown", last, "action");
	}	

	void *ret = ywidNewWidget(e, "text-screen");
	/* printf("%s\n", yeToCStr(e, -1, YE_FORMAT_PRETTY)); */
	return ret;
}
