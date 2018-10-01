/*
 *  RogueMain.c
 *  Brogue
 *
 *  Created by Brian Walker on 12/26/08.
 *  Copyright 2012. All rights reserved.
 *  
 *  This file is part of Brogue.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rogue.h"
#include "IncludeGlobals.h"
#include <math.h>
#include <time.h>

void rogueMain() {
	previousGameSeed = 0;
	initializeBrogueSaveLocation();
	mainBrogueJunction();
}

void executeEvent(rogueEvent *theEvent) {
	rogue.playbackBetweenTurns = false;
	if (theEvent->eventType == KEYSTROKE) {
		executeKeystroke(theEvent->param1, theEvent->controlKey, theEvent->shiftKey);
	} else if (theEvent->eventType == MOUSE_UP
			   || theEvent->eventType == RIGHT_MOUSE_UP) {
		executeMouseClick(theEvent);
	}
}

boolean fileExists(const char *pathname) {
	FILE *openedFile;
	openedFile = fopen(pathname, "rb");
	if (openedFile) {
		fclose(openedFile);
		return true;
	} else {
		return false;
	}
}

// Player specifies a file; if all goes well, put it into path and return true.
// Otherwise, return false.
boolean chooseFile(char *path, char *prompt, char *defaultName, char *suffix) {
	
	if (getInputTextString(path,
						   prompt,
						   min(DCOLS-25, BROGUE_FILENAME_MAX - strlen(suffix)),
						   defaultName,
						   suffix,
						   TEXT_INPUT_FILENAME,
						   false)
		&& path[0] != '\0') {
		
		strcat(path, suffix);
		return true;
	} else {
		return false;
	}
}

// If the file exists, copy it into currentFilePath. (Otherwise return false.)
// Then, strip off the suffix, replace it with ANNOTATION_SUFFIX,
// and if that file exists, copy that into annotationPathname. Return true.
boolean openFile(const char *path) {
	short i;
	char buf[BROGUE_FILENAME_MAX];
	boolean retval;
	
	if (fileExists(path)) {
		
		strcpy(currentFilePath, path);
		annotationPathname[0] = '\0';
		
		// Clip off the suffix.
		strcpy(buf, path);
		for (i = strlen(path); buf[i] != '.' && i > 0; i--) continue;
		if (buf[i] == '.'
			&& i + strlen(ANNOTATION_SUFFIX) < BROGUE_FILENAME_MAX) {
			
			buf[i] = '\0'; // Snip!
			strcat(buf, ANNOTATION_SUFFIX);
			strcpy(annotationPathname, buf); // Load the annotations file too.
		}
		retval = true;
	} else {
		retval = false;
	}
	
	return retval;
}

void benchmark() {
    short i, j, k;
    const color sparklesauce = {10,	0, 20,	60,	40,	100, 30, true};
    uchar theChar;
    
    unsigned long initialTime = (unsigned long) time(NULL);
    for (k=0; k<500; k++) {
        for (i=0; i<COLS; i++) {
            for (j=0; j<ROWS; j++) {
                theChar = rand_range('!', '~');
                plotCharWithColor(theChar, i, j, &sparklesauce, &sparklesauce);
            }
        }
        pauseBrogue(1);
    }
    printf("\n\nBenchmark took a total of %lu seconds.", ((unsigned long) time(NULL)) - initialTime);
}

void welcome() {
    char buf[DCOLS*3], buf2[DCOLS*3];
	message("Hello and welcome, adventurer, to the Dungeons of Doom!", false);
    strcpy(buf, "Retrieve the ");
    encodeMessageColor(buf, strlen(buf), &itemMessageColor);
    strcat(buf, "Amulet of Yendor");
    encodeMessageColor(buf, strlen(buf), &white);
    sprintf(buf2, " from the %ith floor and escape with it!", AMULET_LEVEL);
    strcat(buf, buf2);
	message(buf, false);
    if (KEYBOARD_LABELS) {
        messageWithColor("Press <?> for help at any time.", &backgroundMessageColor, false);
    }
	flavorMessage("The doors to the dungeon slam shut behind you.");
}

void generateFontFiles() {
    short i, j;
	uchar k;
    
	uchar c8[16] = {
		FLOOR_CHAR,
		CHASM_CHAR,
		TRAP_CHAR,
		FIRE_CHAR,
		FOLIAGE_CHAR,
		AMULET_CHAR,
		SCROLL_CHAR,
		RING_CHAR,
		WEAPON_CHAR,
		GEM_CHAR,
		TOTEM_CHAR,
		TURRET_CHAR,
		BAD_MAGIC_CHAR,
		GOOD_MAGIC_CHAR,
		' ',
		' ',
	};
	uchar c9[16] = {
		UP_ARROW_CHAR,
		DOWN_ARROW_CHAR,
		LEFT_ARROW_CHAR,
		RIGHT_ARROW_CHAR,
		UP_TRIANGLE_CHAR,
		DOWN_TRIANGLE_CHAR,
		OMEGA_CHAR,
		THETA_CHAR,
		LAMDA_CHAR,
		KOPPA_CHAR,
		LOZENGE_CHAR,
		CROSS_PRODUCT_CHAR,
		' ',
		' ',
		' ',
		' ',
	};
	
	for (i=0; i<COLS; i++) {
		for(j=0; j<ROWS; j++ ) {
			plotCharWithColor(' ', i, j, &white, &white);
		}
	}
	i = j = 0;
	for (k=0; k<256; k++) {
		i = k % 16;
		j = k / 16;
		if (j >= ROWS) {
			break;
		}
		if (j == 8) {
			plotCharWithColor(c8[i], i, j+5, &white, &black);
		} else if (j == 9) {
			plotCharWithColor(c9[i], i, j+5, &white, &black);
		} else {
			plotCharWithColor(k, i, j+5, &white, &black);
		}
	}
	for (;;) {
		waitForAcknowledgment();
	}
}

// Seed is used as the dungeon seed unless it's zero, in which case generate a new one.
// Either way, previousGameSeed is set to the seed we use.
// None of this seed stuff is applicable if we're playing a recording.
void initializeRogue(unsigned long seed, boolean scumming) {
	short i, j, k;
	item *theItem;
	boolean playingback, playbackFF, playbackPaused;
    short oldRNG;
	
	// generate libtcod font bitmap
	// add any new unicode characters here to include them
#ifdef GENERATE_FONT_FILES
    generateFontFiles();
#endif
	
	playingback = rogue.playbackMode; // the only three animals that need to go on the ark
	playbackPaused = rogue.playbackPaused;
	playbackFF = rogue.playbackFastForward;
	memset((void *) &rogue, 0, sizeof( playerCharacter )); // the flood
	rogue.playbackMode = playingback;
	rogue.playbackPaused = playbackPaused;
	rogue.playbackFastForward = playbackFF;
	
	rogue.gameHasEnded = false;
	rogue.highScoreSaved = false;
	rogue.cautiousMode = false;
	rogue.milliseconds = 0;
	
	rogue.RNG = RNG_SUBSTANTIVE;
	if (!rogue.playbackMode) {
		rogue.seed = seedRandomGenerator(seed);
		previousGameSeed = rogue.seed;
	}
    
    //benchmark();
    if (!scumming)
	initRecording();
	
    levels = malloc(sizeof(levelData) * (DEEPEST_LEVEL+1));
	levels[0].upStairsLoc[0] = (DCOLS - 1) / 2 - 1;
	levels[0].upStairsLoc[1] = DROWS - 2;
	
	// reset enchant and gain strength frequencies
    rogue.lifePotionFrequency = 0;
	rogue.strengthPotionFrequency = 40;
	rogue.enchantScrollFrequency = 60;
	
	// all DF messages are eligible for display
	resetDFMessageEligibility();
	
	// initialize the levels list
	for (i=0; i<DEEPEST_LEVEL+1; i++) {
		levels[i].levelSeed = (unsigned long) rand_range(0, 9999);
        levels[i].levelSeed += (unsigned long) 10000 * rand_range(0, 9999);
		levels[i].monsters = NULL;
		levels[i].dormantMonsters = NULL;
		levels[i].items = NULL;
		levels[i].visited = false;
		levels[i].playerExitedVia[0] = 0;
		levels[i].playerExitedVia[1] = 0;
		do {
			levels[i].downStairsLoc[0] = rand_range(1, DCOLS - 2);
			levels[i].downStairsLoc[1] = rand_range(1, DROWS - 2);
		} while (distanceBetween(levels[i].upStairsLoc[0], levels[i].upStairsLoc[1],
								 levels[i].downStairsLoc[0], levels[i].downStairsLoc[1]) < DCOLS / 3);
		if (i < DEEPEST_LEVEL) {
			levels[i+1].upStairsLoc[0] = levels[i].downStairsLoc[0];
			levels[i+1].upStairsLoc[1] = levels[i].downStairsLoc[1];
		}
	}
    
    // initialize the waypoints list
    for (i=0; i<MAX_WAYPOINT_COUNT; i++) {
        rogue.wpDistance[i] = allocGrid();
        fillGrid(rogue.wpDistance[i], 0);
    }
	
	rogue.rewardRoomsGenerated = 0;
	
	// pre-shuffle the random terrain colors
    oldRNG = rogue.RNG;
    rogue.RNG = RNG_COSMETIC;
	//assureCosmeticRNG;
	for (i=0; i<DCOLS; i++) {
		for( j=0; j<DROWS; j++ ) {
			for (k=0; k<8; k++) {
				terrainRandomValues[i][j][k] = rand_range(0, 1000);
			}
		}
	}
	restoreRNG;
    if (!scumming)
	zeroOutGrid(displayDetail);
	
	for (i=0; i<NUMBER_MONSTER_KINDS; i++) {
		monsterCatalog[i].monsterID = i;
	}
	
	shuffleFlavors();
    
    for (i = 0; i < FEAT_COUNT; i++) {
        rogue.featRecord[i] = featTable[i].initialValue;
    }
	
	deleteMessages();
	for (i = 0; i < MESSAGE_ARCHIVE_LINES; i++) { // Clear the message archive.
		messageArchive[i][0] = '\0';
	}
	messageArchivePosition = 0;
	
	// Seed the stacks.
	floorItems = (item *) malloc(sizeof(item));
	memset(floorItems, '\0', sizeof(item));
	floorItems->nextItem = NULL;
	
    packItems = (item *) malloc(sizeof(item));
	memset(packItems, '\0', sizeof(item));
	packItems->nextItem = NULL;
    
    monsterItemsHopper = (item *) malloc(sizeof(item));
    memset(monsterItemsHopper, '\0', sizeof(item));
    monsterItemsHopper->nextItem = NULL;
    
    for (i = 0; i < MAX_ITEMS_IN_MONSTER_ITEMS_HOPPER; i++) {
        theItem = generateItem(ALL_ITEMS & ~FOOD, -1); // Monsters can't carry food: the food clock cannot be cheated!
        theItem->nextItem = monsterItemsHopper->nextItem;
        monsterItemsHopper->nextItem = theItem;
    }
    
	monsters = (creature *) malloc(sizeof(creature));
	memset(monsters, '\0', sizeof(creature));
    monsters->nextCreature = NULL;
	
	dormantMonsters = (creature *) malloc(sizeof(creature));
	memset(dormantMonsters, '\0', sizeof(creature));
	dormantMonsters->nextCreature = NULL;
    
    graveyard = (creature *) malloc(sizeof(creature));
	memset(graveyard, '\0', sizeof(creature));
	graveyard->nextCreature = NULL;
    
    purgatory = (creature *) malloc(sizeof(creature));
	memset(purgatory, '\0', sizeof(creature));
	purgatory->nextCreature = NULL;
	
	scentMap			= NULL;
	safetyMap			= allocGrid();
	allySafetyMap		= allocGrid();
	chokeMap			= allocGrid();
	
	rogue.mapToSafeTerrain = allocGrid();
	
	// Zero out the dynamic grids, as an essential safeguard against OOSes:
	fillGrid(safetyMap, 0);
	fillGrid(allySafetyMap, 0);
	fillGrid(chokeMap, 0);
	fillGrid(rogue.mapToSafeTerrain, 0);
	
	// initialize the player
	
	memset(&player, '\0', sizeof(creature));
	player.info = monsterCatalog[0];
	initializeGender(&player);
	player.movementSpeed = player.info.movementSpeed;
	player.attackSpeed = player.info.attackSpeed;
	clearStatus(&player);
	player.carriedItem = NULL;
	player.status[STATUS_NUTRITION] = player.maxStatus[STATUS_NUTRITION] = STOMACH_SIZE;
	player.currentHP = player.info.maxHP;
	player.creatureState = MONSTER_ALLY;
	player.ticksUntilTurn = 0;
    player.mutationIndex = -1;
	
	rogue.depthLevel = 1;
    rogue.deepestLevel = 1;
	rogue.scentTurnNumber = 1000;
	rogue.playerTurnNumber = 0;
    rogue.absoluteTurnNumber = 0;
    rogue.previousPoisonPercent = 0;
	rogue.foodSpawned = 0;
    rogue.lifePotionsSpawned = 0;
	rogue.gold = 0;
	rogue.goldGenerated = 0;
	rogue.disturbed = false;
	rogue.autoPlayingLevel = false;
    rogue.automationActive = false;
    rogue.justRested = false;
    rogue.justSearched = false;
	rogue.easyMode = false;
	rogue.inWater = false;
	rogue.creaturesWillFlashThisTurn = false;
	rogue.updatedSafetyMapThisTurn = false;
	rogue.updatedAllySafetyMapThisTurn = false;
	rogue.updatedMapToSafeTerrainThisTurn = false;
	rogue.updatedMapToShoreThisTurn = false;
	rogue.strength = 12;
	rogue.weapon = NULL;
	rogue.armor = NULL;
	rogue.ringLeft = NULL;
	rogue.ringRight = NULL;
	rogue.monsterSpawnFuse = rand_range(125, 175);
	rogue.ticksTillUpdateEnvironment = 100;
	rogue.mapToShore = NULL;
	rogue.cursorLoc[0] = rogue.cursorLoc[1] = -1;
	rogue.xpxpThisTurn = 0;
    
    rogue.yendorWarden = NULL;
    
    rogue.flares = NULL;
    rogue.flareCount = rogue.flareCapacity = 0;
	
	rogue.minersLight = lightCatalog[MINERS_LIGHT];
	
	rogue.clairvoyance = rogue.regenerationBonus
	= rogue.stealthBonus = rogue.transference = rogue.wisdomBonus = rogue.reaping = 0;
	rogue.lightMultiplier = 1;
	
	theItem = generateItem(FOOD, RATION);
	theItem = addItemToPack(theItem);
	
	theItem = generateItem(WEAPON, DAGGER);
	theItem->enchant1 = theItem->enchant2 = 0;
	theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC);
	identify(theItem);
	theItem = addItemToPack(theItem);
	equipItem(theItem, false);
	
	theItem = generateItem(WEAPON, DART);
	theItem->enchant1 = theItem->enchant2 = 0;
	theItem->quantity = 15;
	theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC);
	identify(theItem);
	theItem = addItemToPack(theItem);
	
	theItem = generateItem(ARMOR, LEATHER_ARMOR);
	theItem->enchant1 = 0;
	theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC);
	identify(theItem);
	theItem = addItemToPack(theItem);
	equipItem(theItem, false);
    player.status[STATUS_DONNING] = 0;
    
    recalculateEquipmentBonuses();
	
	DEBUG {
		theItem = generateItem(RING, RING_CLAIRVOYANCE);
		theItem->enchant1 = max(DROWS, DCOLS);
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WEAPON, DAGGER);
		theItem->enchant1 = 50;
		theItem->enchant2 = W_QUIETUS;
		theItem->flags &= ~(ITEM_CURSED);
		theItem->flags |= (ITEM_PROTECTED | ITEM_RUNIC | ITEM_RUNIC_HINTED);
		theItem->damage.lowerBound = theItem->damage.upperBound = 25;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(ARMOR, LEATHER_ARMOR);
		theItem->enchant1 = 50;
		theItem->enchant2 = A_REFLECTION;
		theItem->flags &= ~(ITEM_CURSED | ITEM_RUNIC_HINTED);
		theItem->flags |= (ITEM_PROTECTED | ITEM_RUNIC);
		identify(theItem);
        theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_FIRE);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
        theItem = addItemToPack(theItem);
        
        theItem = generateItem(STAFF, STAFF_LIGHTNING);
        theItem->enchant1 = 10;
        theItem->charges = 300;
        theItem->flags &= ~ITEM_CURSED;
        identify(theItem);
        theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_BLINKING);
		theItem->enchant1 = theItem->charges = 10;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_TUNNELING);
		theItem->enchant1 = 10;
		theItem->charges = 3000;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_OBSTRUCTION);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_BECKONING);
		theItem->charges = 3000;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_ENTRANCEMENT);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_HEALING);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_CONJURATION);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		identify(theItem);
        theItem = addItemToPack(theItem);
		
		theItem = generateItem(STAFF, STAFF_POISON);
		theItem->enchant1 = 10;
		theItem->charges = 300;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_DOMINATION);
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_POLYMORPH);
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_PLENTY);
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
		theItem = addItemToPack(theItem);
		
		theItem = generateItem(WAND, WAND_NEGATION);
		theItem->charges = 300;
		theItem->flags &= ~ITEM_CURSED;
		identify(theItem);
        theItem = addItemToPack(theItem);
        
        theItem = generateItem(RING, RING_AWARENESS);
        theItem->enchant1 = 30;
        theItem->flags &= ~ITEM_CURSED;
        identify(theItem);
        theItem = addItemToPack(theItem);
        
//		short i;
//		for (i=0; i < NUMBER_CHARM_KINDS && i < 4; i++) {
//			theItem = generateItem(CHARM, i);
//			theItem = addItemToPack(theItem);
//		}
	}
	if (!scumming) {
	blackOutScreen();
	welcome();
}
}

// call this once per level to set all the dynamic colors as a function of depth
void updateColors() {
	short i;
	
	for (i=0; i<NUMBER_DYNAMIC_COLORS; i++) {
		*(dynamicColors[i][0]) = *(dynamicColors[i][1]);
		applyColorAverage(dynamicColors[i][0], dynamicColors[i][2], min(100, max(0, rogue.depthLevel * 100 / AMULET_LEVEL)));
	}
}

void startLevel(short oldLevelNumber, short stairDirection, boolean scumming) {
	unsigned long oldSeed;
	item *theItem;
	short loc[2], i, j, x, y, px, py, flying, dir;
	boolean placedPlayer;
	creature *monst;
	enum dungeonLayers layer;
	unsigned long timeAway;
	short **mapToStairs;
	short **mapToPit;
	boolean connectingStairsDiscovered;
    
    if (oldLevelNumber == DEEPEST_LEVEL && stairDirection != -1) {
        return;
    }
    
    synchronizePlayerTimeState();
    
    rogue.updatedSafetyMapThisTurn			= false;
    rogue.updatedAllySafetyMapThisTurn		= false;
    rogue.updatedMapToSafeTerrainThisTurn	= false;
	
	rogue.cursorLoc[0] = -1;
	rogue.cursorLoc[1] = -1;
	rogue.lastTarget = NULL;
	
    connectingStairsDiscovered = (pmap[rogue.downLoc[0]][rogue.downLoc[1]].flags & (DISCOVERED | MAGIC_MAPPED) ? true : false);
	if (stairDirection == 0) { // fallen
		levels[oldLevelNumber-1].playerExitedVia[0] = player.xLoc;
		levels[oldLevelNumber-1].playerExitedVia[1] = player.yLoc;
	}
	
	if (oldLevelNumber != rogue.depthLevel) {
		px = player.xLoc;
		py = player.yLoc;
		if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_AUTO_DESCENT)) {
			for (i=0; i<8; i++) {
				if (!cellHasTerrainFlag(player.xLoc+nbDirs[i][0], player.yLoc+nbDirs[i][1], (T_PATHING_BLOCKER))) {
					px = player.xLoc+nbDirs[i][0];
					py = player.yLoc+nbDirs[i][1];
					break;
				}
			}
		}
		mapToStairs = allocGrid();
		fillGrid(mapToStairs, 0);
		for (flying = 0; flying <= 1; flying++) {
			fillGrid(mapToStairs, 0);
			calculateDistances(mapToStairs, px, py, (flying ? T_OBSTRUCTS_PASSABILITY : T_PATHING_BLOCKER) | T_SACRED, NULL, true, true);
			for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
				x = monst->xLoc;
				y = monst->yLoc;
				if (((monst->creatureState == MONSTER_TRACKING_SCENT && (stairDirection != 0 || monst->status[STATUS_LEVITATING]))
					 || monst->creatureState == MONSTER_ALLY || monst == rogue.yendorWarden)
					&& (stairDirection != 0 || monst->currentHP > 10 || monst->status[STATUS_LEVITATING])
					&& ((flying != 0) == ((monst->status[STATUS_LEVITATING] != 0)
										  || cellHasTerrainFlag(x, y, T_PATHING_BLOCKER)
										  || cellHasTerrainFlag(px, py, T_AUTO_DESCENT)))
					&& !(monst->bookkeepingFlags & MB_CAPTIVE)
					&& !(monst->info.flags & (MONST_WILL_NOT_USE_STAIRS | MONST_RESTRICTED_TO_LIQUID))
					&& !(cellHasTerrainFlag(x, y, T_OBSTRUCTS_PASSABILITY))
					&& !monst->status[STATUS_ENTRANCED]
					&& !monst->status[STATUS_PARALYZED]
					&& (mapToStairs[monst->xLoc][monst->yLoc] < 30000 || monst->creatureState == MONSTER_ALLY || monst == rogue.yendorWarden)) {
					
					monst->status[STATUS_ENTERS_LEVEL_IN] = clamp(mapToStairs[monst->xLoc][monst->yLoc] * monst->movementSpeed / 100 + 1, 1, 150);
					switch (stairDirection) {
						case 1:
							monst->bookkeepingFlags |= MB_APPROACHING_DOWNSTAIRS;
							break;
						case -1:
							monst->bookkeepingFlags |= MB_APPROACHING_UPSTAIRS;
							break;
						case 0:
							monst->bookkeepingFlags |= MB_APPROACHING_PIT;
							break;
						default:
							break;
					}
				}
			}
		}
		freeGrid(mapToStairs);
	}
	
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		if (monst->mapToMe) {
			freeGrid(monst->mapToMe);
			monst->mapToMe = NULL;
		}
		if (monst->safetyMap) {
			freeGrid(monst->safetyMap);
			monst->safetyMap = NULL;
		}
	}
	levels[oldLevelNumber-1].monsters = monsters->nextCreature;
	levels[oldLevelNumber-1].dormantMonsters = dormantMonsters->nextCreature;
	levels[oldLevelNumber-1].items = floorItems->nextItem;
	
	for (i=0; i<DCOLS; i++) {
		for (j=0; j<DROWS; j++) {
            if (pmap[i][j].flags & VISIBLE) {
                // Remember visible cells upon exiting.
                storeMemories(i, j);
            }
			for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
				levels[oldLevelNumber - 1].mapStorage[i][j].layers[layer] = pmap[i][j].layers[layer];
			}
			levels[oldLevelNumber - 1].mapStorage[i][j].volume = pmap[i][j].volume;
			levels[oldLevelNumber - 1].mapStorage[i][j].flags = (pmap[i][j].flags & PERMANENT_TILE_FLAGS);
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedAppearance = pmap[i][j].rememberedAppearance;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedTerrain = pmap[i][j].rememberedTerrain;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedItemCategory = pmap[i][j].rememberedItemCategory;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedItemKind = pmap[i][j].rememberedItemKind;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedCellFlags = pmap[i][j].rememberedCellFlags;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedTerrainFlags = pmap[i][j].rememberedTerrainFlags;
			levels[oldLevelNumber - 1].mapStorage[i][j].rememberedTMFlags = pmap[i][j].rememberedTMFlags;
			levels[oldLevelNumber - 1].mapStorage[i][j].machineNumber = pmap[i][j].machineNumber;
		}
	}
	
	levels[oldLevelNumber - 1].awaySince = rogue.absoluteTurnNumber;
	
	//	Prepare the new level
    if (!scumming)
	{
		rogue.minersLightRadius = DCOLS - 1 << FP_BASE;
		for (i = 0; i < rogue.depthLevel; i++) {
			rogue.minersLightRadius = rogue.minersLightRadius * 85 / 100;
		}
		rogue.minersLightRadius += (225 << FP_BASE)/100;
		updateColors();
		updateRingBonuses(); // also updates miner's light
	}
	
	if (!levels[rogue.depthLevel - 1].visited) { // level has not already been visited
        levels[rogue.depthLevel - 1].scentMap = allocGrid();
        scentMap = levels[rogue.depthLevel - 1].scentMap;
        fillGrid(levels[rogue.depthLevel - 1].scentMap, 0);
		// generate new level
		oldSeed = (unsigned long) rand_range(0, 9999);
        oldSeed += (unsigned long) 10000 * rand_range(0, 9999);
		seedRandomGenerator(levels[rogue.depthLevel - 1].levelSeed);
		
		// Load up next level's monsters and items, since one might have fallen from above.
		monsters->nextCreature			= levels[rogue.depthLevel-1].monsters;
		dormantMonsters->nextCreature	= levels[rogue.depthLevel-1].dormantMonsters;
		floorItems->nextItem			= levels[rogue.depthLevel-1].items;
		
		levels[rogue.depthLevel-1].monsters = NULL;
		levels[rogue.depthLevel-1].dormantMonsters = NULL;
		levels[rogue.depthLevel-1].items = NULL;
		
		digDungeon();
		initializeLevel();
		setUpWaypoints();
		
		shuffleTerrainColors(100, false);
		
        // If we somehow failed to generate the amulet altar,
        // just toss an amulet in there somewhere.
        // It'll be fiiine!
		if (rogue.depthLevel == AMULET_LEVEL
            && !numberOfMatchingPackItems(AMULET, 0, 0, false)
			&& levels[rogue.depthLevel-1].visited == false) {
            
			for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
				if (theItem->category & AMULET) {
					break;
				}
			}
            for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
                if (monst->carriedItem
                    && (monst->carriedItem->category & AMULET)) {
                    
                    theItem = monst->carriedItem;
                    break;
                }
            }
			if (!theItem) {
				placeItem(generateItem(AMULET, 0), 0, 0);
			}
		}
		seedRandomGenerator(oldSeed);
		
		//logLevel();
		
		// Simulate 50 turns so the level is broken in (swamp gas accumulating, brimstone percolating, etc.).
		if (!scumming) { timeAway = 50; }
		else { timeAway = 0; }
		
	} else { // level has already been visited
		
		// restore level
        scentMap = levels[rogue.depthLevel - 1].scentMap;
		timeAway = clamp(0, rogue.absoluteTurnNumber - levels[rogue.depthLevel - 1].awaySince, 30000);
		
		for (i=0; i<DCOLS; i++) {
			for (j=0; j<DROWS; j++) {
				for (layer = 0; layer < NUMBER_TERRAIN_LAYERS; layer++) {
					pmap[i][j].layers[layer] = levels[rogue.depthLevel - 1].mapStorage[i][j].layers[layer];
				}
				pmap[i][j].volume = levels[rogue.depthLevel - 1].mapStorage[i][j].volume;
				pmap[i][j].flags = (levels[rogue.depthLevel - 1].mapStorage[i][j].flags & PERMANENT_TILE_FLAGS);
				pmap[i][j].rememberedAppearance = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedAppearance;
				pmap[i][j].rememberedTerrain = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedTerrain;
				pmap[i][j].rememberedItemCategory = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedItemCategory;
				pmap[i][j].rememberedItemKind = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedItemKind;
				pmap[i][j].rememberedCellFlags = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedCellFlags;
				pmap[i][j].rememberedTerrainFlags = levels[rogue.depthLevel - 1].mapStorage[i][j].rememberedTerrainFlags;
				pmap[i][j].machineNumber = levels[rogue.depthLevel - 1].mapStorage[i][j].machineNumber;
			}
		}
		
		setUpWaypoints();
		
		rogue.downLoc[0]	= levels[rogue.depthLevel - 1].downStairsLoc[0];
		rogue.downLoc[1]	= levels[rogue.depthLevel - 1].downStairsLoc[1];
		rogue.upLoc[0]		= levels[rogue.depthLevel - 1].upStairsLoc[0];
		rogue.upLoc[1]		= levels[rogue.depthLevel - 1].upStairsLoc[1];
		
		monsters->nextCreature = levels[rogue.depthLevel - 1].monsters;
		dormantMonsters->nextCreature = levels[rogue.depthLevel - 1].dormantMonsters;
		floorItems->nextItem = levels[rogue.depthLevel - 1].items;
		
		levels[rogue.depthLevel-1].monsters = NULL;
		levels[rogue.depthLevel-1].dormantMonsters = NULL;
		levels[rogue.depthLevel-1].items = NULL;
		
		for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
			restoreItem(theItem);
		}
		
		mapToStairs = allocGrid();
		mapToPit = allocGrid();
		fillGrid(mapToStairs, 0);
		fillGrid(mapToPit, 0);
		calculateDistances(mapToStairs, player.xLoc, player.yLoc, T_PATHING_BLOCKER, NULL, true, true);
		calculateDistances(mapToPit, levels[rogue.depthLevel-1].playerExitedVia[0],
						   levels[rogue.depthLevel-1].playerExitedVia[0], T_PATHING_BLOCKER, NULL, true, true);
		for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
			restoreMonster(monst, mapToStairs, mapToPit);
		}
		freeGrid(mapToStairs);
		freeGrid(mapToPit);
	}
	
	// Simulate the environment!
	// First bury the player in limbo while we run the simulation,
	// so that any harmful terrain doesn't affect her during the process.
	px = player.xLoc;
	py = player.yLoc;
	player.xLoc = player.yLoc = 0;
	for (i = 0; i < 100 && i < (short) timeAway; i++) {
		updateEnvironment();
	}
	player.xLoc = px;
	player.yLoc = py;
	
    if (!levels[rogue.depthLevel-1].visited) {
        levels[rogue.depthLevel-1].visited = true;
        if (rogue.depthLevel == AMULET_LEVEL) {
            messageWithColor("An alien energy permeates the area. The Amulet of Yendor must be nearby!", &itemMessageColor, false);
        } else if (rogue.depthLevel == DEEPEST_LEVEL) {
            messageWithColor("An overwhelming sense of peace and tranquility settles upon you.", &lightBlue, false);
        }
    }
	
	// Position the player.
	if (stairDirection == 0) { // fell into the level
		
		getQualifyingLocNear(loc, player.xLoc, player.yLoc, true, 0,
							 (T_PATHING_BLOCKER),
							 (HAS_MONSTER | HAS_ITEM | HAS_STAIRS | IS_IN_MACHINE), false, false);
	} else {
		if (stairDirection == 1) { // heading downward
			player.xLoc = rogue.upLoc[0];
			player.yLoc = rogue.upLoc[1];
		} else if (stairDirection == -1) { // heading upward
			player.xLoc = rogue.downLoc[0];
			player.yLoc = rogue.downLoc[1];
		}
        
        placedPlayer = false;
        for (dir=0; dir<4 && !placedPlayer; dir++) {
            loc[0] = player.xLoc + nbDirs[dir][0];
            loc[1] = player.yLoc + nbDirs[dir][1];
            if (!cellHasTerrainFlag(loc[0], loc[1], T_PATHING_BLOCKER)
                && !(pmap[loc[0]][loc[1]].flags & (HAS_MONSTER | HAS_ITEM | HAS_STAIRS | IS_IN_MACHINE))) {
                placedPlayer = true;
            }
        }
		if (!placedPlayer) {
            getQualifyingPathLocNear(&loc[0], &loc[1],
                                     player.xLoc, player.yLoc,
                                     true,
                                     T_DIVIDES_LEVEL, NULL,
                                     T_PATHING_BLOCKER, (HAS_MONSTER | HAS_ITEM | HAS_STAIRS | IS_IN_MACHINE),
                                     false);
        }
	}
	player.xLoc = loc[0];
	player.yLoc = loc[1];
		
	pmap[player.xLoc][player.yLoc].flags |= HAS_PLAYER;
	
	if (connectingStairsDiscovered) {
        for (i = rogue.upLoc[0]-1; i <= rogue.upLoc[0] + 1; i++) {
            for (j = rogue.upLoc[1]-1; j <= rogue.upLoc[1] + 1; j++) {
                if (coordinatesAreInMap(i, j)) {
                    discoverCell(i, j);
                }
            }
        }
	}
	if (cellHasTerrainFlag(player.xLoc, player.yLoc, T_IS_DEEP_WATER) && !player.status[STATUS_LEVITATING]
		&& !cellHasTerrainFlag(player.xLoc, player.yLoc, (T_ENTANGLES | T_OBSTRUCTS_PASSABILITY))) {
		rogue.inWater = true;
	}
    if (!scumming) {
	updateMapToShore();
	updateVision(true);
    rogue.aggroRange = currentAggroValue();
	
	// update monster states so none are hunting if there is no scent and they can't see the player
	for (monst = monsters->nextCreature; monst != NULL; monst = monst->nextCreature) {
		updateMonsterState(monst);
	}
    }
	
	rogue.playbackBetweenTurns = true;
	if (!scumming) {
	displayLevel();
	refreshSideBar(-1, -1, false);
	
	if (rogue.playerTurnNumber) {
		rogue.playerTurnNumber++; // Increment even though no time has passed.
	}
	RNGCheck();
	flushBufferToFile();
    deleteAllFlares(); // So discovering something on the same turn that you fall down a level doesn't flash stuff on the previous level.
    hideCursor();
}
}

//scumming functions

/*
int getExcited(item *theItem, int *excitement)
{
	int excitementdelta = 0;
	if (theItem->category == WEAPON)
	{
		if (theItem->enchant2 == W_QUIETUS)
			excitementdelta = 10;
		else if (theItem->enchant2 == W_PARALYSIS)
			excitementdelta = 10;
		else if (theItem->enchant2 == W_SLOWING)
			excitementdelta = 10;
		else if (theItem->enchant2 == W_CONFUSION)
			excitementdelta = 10;
		else if (theItem->kind == BROADSWORD
				|| theItem->kind == HAMMER
				|| theItem->kind == PIKE
				|| theItem->kind == WAR_AXE)
			excitementdelta = theItem->enchant1 + 1;
	}
	else if (theItem->category == ARMOR)
	{
		if (theItem->kind >= BANDED_MAIL)
		{
			if (theItem->enchant2 == A_ABSORPTION
				|| theItem->enchant2 == A_REPRISAL
				|| theItem->enchant2 == A_REFLECTION)
				excitementdelta = 5;
			else if (theItem->kind == SPLINT_MAIL)
				excitementdelta = theItem->enchant1;
			else if (theItem->kind == PLATE_MAIL)
				excitementdelta = theItem->enchant1 + 3;
		}
	}
	else if (theItem->category == POTION)
	{
		if (theItem->kind == POTION_GAIN_STRENGTH)
			excitementdelta = 1;
		else if (theItem->kind == POTION_GAIN_LEVEL)
			excitementdelta = 1;
	}
	else if (theItem->category == SCROLL)
	{
		if (theItem->kind == SCROLL_ENCHANT_ITEM)
			excitementdelta = 1;
	}
	else if (theItem->category == STAFF)
	{
		excitementdelta = (theItem->enchant1 + 1) / 2;
	}
	else if (theItem->category == WAND && theItem->kind != WAND_INVISIBILITY)
	{
		excitementdelta = 1;
	}
	else if (theItem->category == RING)
	{
		excitementdelta = theItem->enchant1 - 1;
	}
	if (excitementdelta > 0) {
		*excitement += excitementdelta;
		return excitementdelta;
	}
	return 0;
}
*/

boolean isEnchant(item *theItem)
{
    return (theItem->category == SCROLL && theItem->kind == SCROLL_ENCHANTING);
}

boolean isStrength(item *theItem)
{
    return (theItem->category == POTION && theItem->kind == POTION_STRENGTH);
}

boolean isLife(item *theItem)
{
    return (theItem->category == POTION && theItem->kind == POTION_LIFE);
}

boolean isBuff(item *theItem)
{
    return (
               (theItem->category == SCROLL && theItem->kind == SCROLL_ENCHANTING)
            || (theItem->category == POTION && (theItem->kind == POTION_STRENGTH || theItem->kind == POTION_LIFE ))
            );
}

boolean isSeparator(char c)
{
    return (c == ',' || c == ';' || c == '&' || c == '|' || c == '.');
}

unsigned long seedPeer(unsigned long int seed)
{
    initializeRogue(seed, true);
    rogue.playbackOmniscience = true;
    char textbuf[255];
    char itembuf[255];
    char monstbuf[255];
    int enchantcount = 0;
    int strengthcount = 0;
    int lifecount = 0;
    item *theItem;
    //for each floor...
    while (rogue.depthLevel <= AMULET_LEVEL)
    {
        printf("\nDepth %d:\n", rogue.depthLevel);
        startLevel(max(rogue.depthLevel - 1, 1), 1, true);

        //items
        for (theItem = floorItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
            itemName(theItem, itembuf, true, false, NULL);
            if (strstr(itembuf, "gold pieces") == NULL) {
                if (pmap[theItem->xLoc][theItem->yLoc].machineNumber > 0)
                {
                    sprintf(textbuf, " (in machine %d)", pmap[theItem->xLoc][theItem->yLoc].machineNumber);
                    strcat(itembuf, textbuf);
                }
                printf("%s\n", itembuf);
            }
            if (isEnchant(theItem)) ++enchantcount;
            if (isStrength(theItem)) ++strengthcount;
            if (isLife(theItem)) ++lifecount;
        }

        //check for captive allies
        creature *monst;
        for (monst = monsters; monst != NULL; monst = monst->nextCreature)
        {
            //check for items in monsters' pockets too
            if (monst->carriedItem != NULL)
            {
                theItem = monst->carriedItem;
                itemName(theItem, itembuf, true, false, NULL);
                if (strstr(itembuf, "gold pieces") == NULL) {
                    monsterNameWithMutation(monstbuf, monst, false);
                    printf("%s (carried by %s)\n", itembuf, monstbuf);
                }
                if (isEnchant(theItem)) ++enchantcount;
                if (isStrength(theItem)) ++strengthcount;
                if (isLife(theItem)) ++lifecount;
            }
            if (monst->bookkeepingFlags & MB_CAPTIVE)
            {
                monsterNameWithMutation(monstbuf, monst, false);
                printf("captive %s\n", monstbuf);
            }
        }
        rogue.depthLevel++;
    }
    printf("\nIn the monster carried item queue: \n");
    for (theItem = monsterItemsHopper->nextItem; theItem != NULL; theItem = theItem->nextItem) {
        itemName(theItem, itembuf, true, false, NULL);
        if (strstr(itembuf, "gold pieces") == NULL) {
            printf("%s (carried by ???)\n", itembuf);
        }
    }

    rogue.playbackOmniscience = false; emptyGraveyard(); freeEverything();
    printf("\nThat seed had %d enchanting scrolls.\n", enchantcount);
    printf("That seed had %d potions of strength.\n", strengthcount);
    printf("That seed had %d potions of life.\n", lifecount);
    fflush(stdout);
    return 0;
}

unsigned long startScum(char* target, boolean gui, boolean inahurry)
{
    char target2[255]; int i = 0; int j = 0; const int MAX_TARGETS = 16;
    //step 0: lower case
    for (i = 0; target[i]; ++i) { target[i] = tolower(target[i]); }
    strcpy(target2, target);

    //we're going to split char* target into up to MAX_TARGETS targets
    char *targets[MAX_TARGETS];
    boolean matches[MAX_TARGETS]; for (i = 0; i < MAX_TARGETS; ++i) { matches[i] = false;}
    targets[0] = target2;
    short numtargets = 0;

    char *end = target2;
    //get pointer to end of target2
    while (*end != '\0') { ++end; }

    //step 1: find all separators, place positions in an array
    for (i = 0; target2[i]; ++i) {
        if (isSeparator(target2[i]))
        {
            targets[numtargets] = &target2[i];
            //step 2: floodfill around separators into whitespace/separators with nulls
            //starting at separator go forwards
            char *p = targets[numtargets];
            while (*p == ' ' || isSeparator(*p)) { *p = '\1'; ++p; }
            if (p >= end) continue; //make sure we stayed in bounds
            //starting BEFORE separator go backwards
            p = targets[numtargets]; --p;
            while (*p == ' ' || isSeparator(*p)) { *p = '\1'; --p; }
            //NOW we move onto the next one
            ++numtargets;
        }
        if (numtargets == 0) { numtargets = 1; }
    }

    //turn \1 into \0
    for (i = 0; target2[i]; ++i) { if (target2[i] == '\1') target2[i] = '\0'; }

    //step 3: move pointers to separators forward until they point to non-null (= our targets)
    for (i = 0; i < numtargets; ++i)
    {
        while (*targets[i] == '\0') { targets[i]++; }
    }

    boolean enchantcheck = false;
    int enchantexpect = (numtargets > 2) ? 15 : 20;

    //step 4: does the caller care about scrolls of enchantment?
    for (i = 0; i < numtargets; ++i)
    {
        if (strstr(targets[i], "enchan") != NULL)
        {
            enchantcheck = true;
            //search for number
            int tempenchantexpect = 0;
            char *firstnumber = targets[i];
            while (*firstnumber < '0' || *firstnumber > '9') {
                firstnumber++;
            }
            sscanf(firstnumber, "%d", &tempenchantexpect);
            if (tempenchantexpect > 1 && tempenchantexpect < 99) {
                enchantexpect = tempenchantexpect;
            }
            //scrub entry, decrement numtargets, shuffle target pointers along the array
            char *p = targets[i];
            while (*p != '\0') {
                *p = '\0'; ++p;
            }
            for (j = i + 1; j < numtargets; ++j)
            {
                targets[j - 1] = targets[j];
            }
            --numtargets;
            if (!gui) {
                printf("Detected that you care about scrolls of enchanting - %d or more\n", enchantexpect);
            }
            break;
        }
    }

    boolean mutationcheck = false;

    //step 5: is the caller looking for a mutated ally?
    for (i = 0; i < numtargets; ++i)
    {
        for (j = 0; j < NUMBER_MUTATORS; ++j)
        {
            if (strstr(mutationCatalog[j].title, targets[i]) != NULL)
            {
                mutationcheck = true;
                if (!gui) {
                    printf("Detected that you are looking for a mutated ally - %s\n", targets[i]);
                }
                break;
            }
        }
    }

	seedRandomGenerator(0);
	unsigned long startingSeed = rand_range(0, 1<<25);

	short stopAfterDepth = inahurry ? 2 : (enchantcheck || mutationcheck) ? AMULET_LEVEL : (numtargets > 1) ? 7 : 2;
    unsigned long seedsTried = 0;
    unsigned long seedsToTry = (enchantcheck && enchantexpect >= 20) ? 10000 : (numtargets > 1) ? 10000 : (mutationcheck) ? 10000 : 1000;
    char textbuf[255];
    char itembuf[255];
    char monstbuf[255];
    if (!gui) {
        printf("Searching for %ld seeds starting with %ld\n", seedsToTry, startingSeed);
        for (i = 0; i < numtargets; ++i) {
            printf("'%s'\n", targets[i]);
        }
        printf("\n");
    }
    fflush(stdout);
	while (seedsTried < seedsToTry) //plenty
	{

        if (gui) {
            //fancy loading bar
            blackOutScreen();
            sprintf(textbuf, "Searching, Q to abort");
            printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 - 1, &teal, &black, 0);
            sprintf(textbuf, "%ld", seedsTried);
            printProgressBar((COLS - 20) / 2, ROWS / 2, textbuf,
                         seedsTried, (seedsToTry*2) - seedsTried, &darkPurple, false);
            printString(target, (COLS - strLenWithoutEscapes(target)) / 2, ROWS / 2 + 1, &teal, &black, 0);
            //quit, if we wish
            if (pauseBrogue(1))
            {
                rogueEvent theEvent;
                nextBrogueEvent(&theEvent, true, false, true);
                if (theEvent.eventType == KEYSTROKE)
                {
                    char key = theEvent.param1;
                    if (key == ESCAPE_KEY || key == 'Q' || key == 'q')
                    {return 0;}
                }
            }
        }
        for (i = 0; i < MAX_TARGETS; ++i) { matches[i] = false;}

		char itemsList[255][255];
		unsigned char itemsPos = 0;
		int enchantcount = 0;
        int strengthcount = 0;
        int lifecount = 0;
		initializeRogue(startingSeed, true);
		rogue.playbackOmniscience = true;
		//for each floor...
		while (rogue.depthLevel <= stopAfterDepth)
		{
		    if (!gui) { sprintf(itemsList[itemsPos], "Depth %d:", rogue.depthLevel); itemsPos++; }
            startLevel(max(rogue.depthLevel - 1, 1), 1, true);
            //for each item, if it's not cursed/negatively enchanted...
			item *theItem;
			for (theItem = floorItems; theItem != NULL; theItem = theItem->nextItem) {
                if (isEnchant(theItem)) ++enchantcount;
                if (isStrength(theItem)) ++strengthcount;
                if (isLife(theItem)) ++lifecount;
			    if (gui) { if (theItem->flags & (ITEM_CURSED) || theItem->enchant1 < 0 || theItem->enchant2 < 0) continue; }
                theItem->flags = theItem->flags | (ITEM_IDENTIFIED | ITEM_RUNIC_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN);
                itemName(theItem, itembuf, true, false, NULL);
                boolean printed = false;
                //for each of our target items, does a substring match work? when we find one, skip to the next item
                for (i = 0; i < numtargets; ++i)
                {
                    //now each 'token' in the scum entry has to be a substring of the item's full name, not just the entire entry - so you can be abbreviated, get the order wrong, etc
                    char tokens[255]; strncpy(tokens, targets[i], sizeof(tokens));
                    boolean nextitemplease = false;
                    char* pch = strtok(tokens, " ");
                    for (; pch != NULL; pch = strtok(NULL, " "))
                    {
                        //allows for weapon multiplicity, armour multiplicity and so on
                        if (strcmp("weapon", pch) == 0) { if (theItem->category == WEAPON) { continue; } }
                        if (strcmp("armour", pch) == 0 || strcmp("armor", pch) == 0) { if (theItem->category == ARMOR) { continue; } }
                        if (strstr(itembuf, pch) == NULL) { nextitemplease = true; break; }
                    }
                    if (nextitemplease) { continue; }
                    //if (strstr(itembuf, targets[i]) != NULL)
                    {
                        if (!gui && !printed)
                        {
                            if (pmap[theItem->xLoc][theItem->yLoc].machineNumber > 0)
                            {
                                sprintf(textbuf, " (in machine %d)", pmap[theItem->xLoc][theItem->yLoc].machineNumber);
                                strcat(itembuf, textbuf);
                            }
                            sprintf(itemsList[itemsPos], "%s", itembuf);
                            itemsPos++;
                            printed = true;
                        }
                        if (!matches[i]) { matches[i] = true; break; }
                    }
                }
			}
			//check for captive allies
            creature *monst;
            for (monst = monsters; monst != NULL; monst = monst->nextCreature)
            {
                if (monst->bookkeepingFlags & MB_CAPTIVE)
                {
                    monsterNameWithMutation(monstbuf, monst, false);
                    boolean printed = false;
                    for (i = 0; i < numtargets; ++i)
                    {
                        //is target a substring of monster, or is monster a substring of target
                        //(and immunity is not a substring of target and slaying is not a substring of target)?
                        if (strstr(monstbuf, targets[i]) != NULL ||
                            (strstr(targets[i], monstbuf) != NULL && strstr(targets[i], "immun") == NULL && strstr(targets[i], "slay") == NULL))
                        {
                            if (!gui && !printed) { sprintf(itemsList[itemsPos], "captive %s", monstbuf); itemsPos++; printed = true;}
                            if (!matches[i]) { matches[i] = true; break; }
                        }
                    }
                }
                else if (strcmp(monst->info.monsterName, "black jelly") == 0 ) //for black jelly scumming
                {
                    boolean printed = false;
                    for (i = 0; i < numtargets; ++i)
                    {
                        if (strstr("black jelly", targets[i]) != NULL)
                        {
                            monsterNameWithMutation(monstbuf, monst, false);
                            if (!gui && !printed) { sprintf(itemsList[itemsPos], "%s", monstbuf); itemsPos++; printed = true;}
                            if (!matches[i]) { matches[i] = true; break; }
                        }
                    }
                }
                //check for items in monsters' pockets too
                if (monst->carriedItem != NULL)
                {
                    theItem = monst->carriedItem;
                    if (isEnchant(theItem)) ++enchantcount;
                    if (isStrength(theItem)) ++strengthcount;
                    if (isLife(theItem)) ++lifecount;
                    if (gui) { if (theItem->flags & (ITEM_CURSED) || theItem->enchant1 < 0 || theItem->enchant2 < 0) continue; }
                    theItem->flags = theItem->flags | (ITEM_IDENTIFIED | ITEM_RUNIC_IDENTIFIED | ITEM_MAX_CHARGES_KNOWN);
                    itemName(theItem, itembuf, true, false, NULL);
                    boolean printed = false;
                    //for each of our target items, does a substring match work? when we find one, skip to the next item
                    for (i = 0; i < numtargets; ++i)
                    {
                        //now each 'token' in the scum entry has to be a substring of the item's full name, not just the entire entry - so you can be abbreviated, get the order wrong, etc
                        char tokens[255]; strncpy(tokens, targets[i], sizeof(tokens));
                        boolean nextitemplease = false;
                        char* pch = strtok(tokens, " ");
                        for (; pch != NULL; pch = strtok(NULL, " "))
                        {
                            //allows for weapon multiplicity, armour multiplicity and so on
                            if (strcmp("weapon", pch) == 0) { if (theItem->category == WEAPON) { continue; } }
                            if (strcmp("armour", pch) == 0 || strcmp("armor", pch) == 0) { if (theItem->category == ARMOR) { continue; } }
                            if (strstr(itembuf, pch) == NULL) { nextitemplease = true; break; }
                        }
                        if (nextitemplease) { continue; }
                        //if (strstr(itembuf, targets[i]) != NULL)
                        {
                            monsterNameWithMutation(monstbuf, monst, false);
                            if (!gui && !printed) { sprintf(itemsList[itemsPos], "%s (carried by %s)", itembuf, monstbuf); itemsPos++; printed = true;}
                            if (!matches[i]) { matches[i] = true; break; }
                        }
                    }
                }
            }

			rogue.depthLevel++;
		}
		rogue.playbackOmniscience = false; emptyGraveyard(); freeEverything();
		boolean allmatched = true;
		for (i = 0; i < numtargets; ++i)
		{
		    if (!matches[i]) { allmatched = false; break;}
		}

		if (allmatched && (!enchantcheck || (enchantcheck && (enchantcount >= enchantexpect)))){
		    if (!gui) {
                printf("I sense seed %ld has what you seek...\n", startingSeed);
                for (i = 0; i < itemsPos; i++)
                {
                    printf("%s\n", itemsList[i]);
                }
                if (enchantcheck) {
                        printf("\nThat seed had %d enchanting scrolls.\n", enchantcount);
                        printf("That seed had %d potions of strength.\n", strengthcount);
                        printf("That seed had %d potions of life.\n", lifecount);
                }
                printf("\n");
                fflush(stdout);
		    }
		    else {
		        blackOutScreen();
		        sprintf(textbuf, "You shall find '%s' in seed %ld", target, startingSeed);
		        displayCenteredAlert(textbuf);
		        sprintf(textbuf, "on the first %d floors...", stopAfterDepth);
		        printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 1, &teal, &black, 0);
		        displayMoreSign();
                return startingSeed;
		    }
        }

		startingSeed++; seedsTried++;
	}

	if (gui) {

	    blackOutScreen();
	    sprintf(textbuf, "Example searches:");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 3, &teal, &black, 0);
	    sprintf(textbuf, "quietus");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 4, &teal, &black, 0);
	    sprintf(textbuf, "+3 axe of quietus");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 5, &teal, &black, 0);
	    sprintf(textbuf, "quietus, plate");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 6, &teal, &black, 0);
	    sprintf(textbuf, "staff of obstruction [4/4]");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 7, &teal, &black, 0);
	    sprintf(textbuf, "wand of plenty [4]");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 8, &teal, &black, 0);
	    sprintf(textbuf, "stealth, war hammer, +3 ring of awareness");
	    printString(textbuf, (COLS - strLenWithoutEscapes(textbuf)) / 2, ROWS / 2 + 9, &teal, &black, 0);
	    sprintf(textbuf, "Didn't find '%s' in %ld seeds. Try again?", target, seedsToTry);
        if (confirm(textbuf, true)) return startScum(target, gui, inahurry);
	 }
	 else {
        printf("Searched %ld seeds. I'm done.\n", seedsTried);
	 }
	return 0;
}

//end seed scumming functions

void freeGlobalDynamicGrid(short ***grid) {
	if (*grid) {
		freeGrid(*grid);
		*grid = NULL;
	}
}

void freeCreature(creature *monst) {
	freeGlobalDynamicGrid(&(monst->mapToMe));
	freeGlobalDynamicGrid(&(monst->safetyMap));
	if (monst->carriedItem) {
		free(monst->carriedItem);
		monst->carriedItem = NULL;
	}
	if (monst->carriedMonster) {
		freeCreature(monst->carriedMonster);
		monst->carriedMonster = NULL;
	}
	free(monst);
}

void emptyGraveyard() {
	creature *monst, *monst2;
	for (monst = graveyard->nextCreature; monst != NULL; monst = monst2) {
		monst2 = monst->nextCreature;
		freeCreature(monst);
	}
	graveyard->nextCreature = NULL;
}

void freeEverything() {
	short i;
	creature *monst, *monst2;
	item *theItem, *theItem2;
	
#ifdef AUDIT_RNG
	fclose(RNGLogFile);
#endif
    
	freeGlobalDynamicGrid(&safetyMap);
	freeGlobalDynamicGrid(&allySafetyMap);
	freeGlobalDynamicGrid(&chokeMap);
	freeGlobalDynamicGrid(&rogue.mapToShore);
	freeGlobalDynamicGrid(&rogue.mapToSafeTerrain);
	
	for (i=0; i<DEEPEST_LEVEL+1; i++) {
		for (monst = levels[i].monsters; monst != NULL; monst = monst2) {
			monst2 = monst->nextCreature;
			freeCreature(monst);
		}
		levels[i].monsters = NULL;
		for (monst = levels[i].dormantMonsters; monst != NULL; monst = monst2) {
			monst2 = monst->nextCreature;
			freeCreature(monst);
		}
		levels[i].dormantMonsters = NULL;
		for (theItem = levels[i].items; theItem != NULL; theItem = theItem2) {
			theItem2 = theItem->nextItem;
			deleteItem(theItem);
		}
		levels[i].items = NULL;
        if (levels[i].scentMap) {
            freeGrid(levels[i].scentMap);
            levels[i].scentMap = NULL;
        }
	}
    scentMap = NULL;
    for (monst = monsters; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    monsters = NULL;
    for (monst = dormantMonsters; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    dormantMonsters = NULL;
    for (monst = graveyard; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    graveyard = NULL;
    for (monst = purgatory; monst != NULL; monst = monst2) {
        monst2 = monst->nextCreature;
        freeCreature(monst);
    }
    purgatory = NULL;
    for (theItem = floorItems; theItem != NULL; theItem = theItem2) {
        theItem2 = theItem->nextItem;
        deleteItem(theItem);
    }
    floorItems = NULL;
    for (theItem = packItems; theItem != NULL; theItem = theItem2) {
        theItem2 = theItem->nextItem;
        deleteItem(theItem);
    }
    packItems = NULL;
    for (theItem = monsterItemsHopper; theItem != NULL; theItem = theItem2) {
        theItem2 = theItem->nextItem;
        deleteItem(theItem);
    }
    monsterItemsHopper = NULL;
    for (i=0; i<MAX_WAYPOINT_COUNT; i++) {
        freeGrid(rogue.wpDistance[i]);
    }
    
    deleteAllFlares();
    if (rogue.flares) {
        free(rogue.flares);
        rogue.flares = NULL;
    }
    
    free(levels);
    levels = NULL;
}

void gameOver(char *killedBy, boolean useCustomPhrasing) {
    short i, y;
	char buf[200], highScoreText[200], buf2[200];
	rogueHighScoresEntry theEntry;
	cellDisplayBuffer dbuf[COLS][ROWS];
	boolean playback;
	rogueEvent theEvent;
    item *theItem;
    
    if (player.bookkeepingFlags & MB_IS_DYING) {
        // we've already been through this once; let's avoid overkill.
        return;
    } else {
        player.bookkeepingFlags |= MB_IS_DYING;
    }
	
	rogue.autoPlayingLevel = false;
	
	flushBufferToFile();
	
	if (rogue.quit) {
		if (rogue.playbackMode) {
			playback = rogue.playbackMode;
			rogue.playbackMode = false;
			message("(The player quit at this point.)", true);
			rogue.playbackMode = playback;
		}
	} else {
		playback = rogue.playbackMode;
		if (!D_IMMORTAL) {
			rogue.playbackMode = false;
		}
        strcpy(buf, "You die...");
        if (KEYBOARD_LABELS) {
            encodeMessageColor(buf, strlen(buf), &veryDarkGray);
            strcat(buf, " (press 'i' to view your inventory)");
        }
        player.currentHP = 0; // So it shows up empty in the side bar.
        refreshSideBar(-1, -1, false);
		messageWithColor(buf, &badMessageColor, false);
        displayMoreSignWithoutWaitingForAcknowledgment();
        
        do {
            nextBrogueEvent(&theEvent, false, false, false);
            if (theEvent.eventType == KEYSTROKE
                && theEvent.param1 != ACKNOWLEDGE_KEY
                && theEvent.param1 != ESCAPE_KEY
                && theEvent.param1 != INVENTORY_KEY) {
                
                flashTemporaryAlert(" -- Press space or click to continue, or press 'i' to view inventory -- ", 1500);
            } else if (theEvent.eventType == KEYSTROKE && theEvent.param1 == INVENTORY_KEY) {
                for (theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
                    identify(theItem);
                    theItem->flags &= ~ITEM_MAGIC_DETECTED;
                }
                displayInventory(ALL_ITEMS, 0, 0, true, false);
            }
        } while (!(theEvent.eventType == KEYSTROKE && (theEvent.param1 == ACKNOWLEDGE_KEY || theEvent.param1 == ESCAPE_KEY)
                   || theEvent.eventType == MOUSE_UP));
        
        confirmMessages();
        
		rogue.playbackMode = playback;
	}
    
    rogue.creaturesWillFlashThisTurn = false;
	
	if (D_IMMORTAL && !rogue.quit) {
		message("...but then you get better.", false);
		player.currentHP = player.info.maxHP;
		if (player.status[STATUS_NUTRITION] < 10) {
			player.status[STATUS_NUTRITION] = STOMACH_SIZE;
		}
		player.bookkeepingFlags &= ~MB_IS_DYING;
		return;
	}
	
	if (rogue.highScoreSaved) {
		return;
	}
	rogue.highScoreSaved = true;
	
	if (rogue.quit) {
		blackOutScreen();
	} else {
		copyDisplayBuffer(dbuf, displayBuffer);
		funkyFade(dbuf, &black, 0, 120, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), false);
	}
	
	if (useCustomPhrasing) {
		sprintf(buf, "%s on depth %i", killedBy, rogue.depthLevel);
	} else {
		sprintf(buf, "Killed by a%s %s on depth %i", (isVowelish(killedBy) ? "n" : ""), killedBy,
				rogue.depthLevel);
	}
    theEntry.score = rogue.gold;
	if (rogue.easyMode) {
		theEntry.score /= 10;
	}
    strcpy(highScoreText, buf);
    if (theEntry.score > 0) {
        sprintf(buf2, " with %li gold", theEntry.score);
        strcat(buf, buf2);
    }
    if (numberOfMatchingPackItems(AMULET, 0, 0, false) > 0) {
        strcat(buf, ", amulet in hand");
    }
    strcat(buf, ".");
    strcat(highScoreText, ".");
	
	strcpy(theEntry.description, highScoreText);
	
	if (!rogue.quit) {
        printString(buf, (COLS - strLenWithoutEscapes(buf)) / 2, ROWS / 2, &gray, &black, 0);
        
        y = ROWS / 2 + 3;
        for (i = 0; i < FEAT_COUNT; i++) {
            //printf("\nConduct %i (%s) is %s.", i, featTable[i].name, rogue.featRecord[i] ? "true" : "false");
            if (rogue.featRecord[i]
                && !featTable[i].initialValue) {
                
                sprintf(buf, "%s: %s", featTable[i].name, featTable[i].description);
                printString(buf, (COLS - strLenWithoutEscapes(buf)) / 2, y, &advancementMessageColor, &black, 0);
                y++;
            }
        }
        
		displayMoreSign();
	}
	
	if (!rogue.playbackMode) {
		if (saveHighScore(theEntry)) {
			printHighScores(true);
		}
		blackOutScreen();
		saveRecording();
	}
	
	rogue.gameHasEnded = true;
}

void victory(boolean superVictory) {
	char buf[COLS*3], victoryVerb[20];
	item *theItem;
	short i, j, gemCount = 0;
	unsigned long totalValue = 0;
	rogueHighScoresEntry theEntry;
	boolean qualified, isPlayback;
	cellDisplayBuffer dbuf[COLS][ROWS];
	
	flushBufferToFile();
	
	deleteMessages();
    if (superVictory) {
        message(    "Light streams through the portal, and you are teleported out of the dungeon.", false);
        copyDisplayBuffer(dbuf, displayBuffer);
        funkyFade(dbuf, &superVictoryColor, 0, 240, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), false);
        displayMoreSign();
        printString("Congratulations; you have transcended the Dungeons of Doom!                 ", mapToWindowX(0), mapToWindowY(-1), &black, &white, 0);
        displayMoreSign();
        clearDisplayBuffer(dbuf);
        deleteMessages();
        strcpy(displayedMessage[0], "You retire in splendor, forever renowned for your remarkable triumph.     ");
    } else {
        message(    "You are bathed in sunlight as you throw open the heavy doors.", false);
        copyDisplayBuffer(dbuf, displayBuffer);
        funkyFade(dbuf, &white, 0, 240, mapToWindowX(player.xLoc), mapToWindowY(player.yLoc), false);
        displayMoreSign();
        printString("Congratulations; you have escaped from the Dungeons of Doom!     ", mapToWindowX(0), mapToWindowY(-1), &black, &white, 0);
        displayMoreSign();
        clearDisplayBuffer(dbuf);
        deleteMessages();
        strcpy(displayedMessage[0], "You sell your treasures and live out your days in fame and glory.");
    }
    
	printString(displayedMessage[0], mapToWindowX(0), mapToWindowY(-1), &white, &black, dbuf);
	
	printString("Gold", mapToWindowX(2), mapToWindowY(1), &white, &black, dbuf);
	sprintf(buf, "%li", rogue.gold);
	printString(buf, mapToWindowX(60), mapToWindowY(1), &itemMessageColor, &black, dbuf);
	totalValue += rogue.gold;
	
	for (i = 4, theItem = packItems->nextItem; theItem != NULL; theItem = theItem->nextItem) {
		if (theItem->category & GEM) {
			gemCount += theItem->quantity;
		}
        if (theItem->category == AMULET && superVictory) {
            printString("The Birthright of Yendor", mapToWindowX(2), min(ROWS-1, i + 1), &itemMessageColor, &black, dbuf);
            sprintf(buf, "%li", max(0, itemValue(theItem) * 2));
            printString(buf, mapToWindowX(60), min(ROWS-1, i + 1), &itemMessageColor, &black, dbuf);
            totalValue += max(0, itemValue(theItem) * 2);
            i++;
        } else if (itemValue(theItem) > 0) {
            identify(theItem);
            itemName(theItem, buf, true, true, &white);
            upperCase(buf);
            printString(buf, mapToWindowX(2), min(ROWS-1, i + 1), &white, &black, dbuf);
            sprintf(buf, "%li", max(0, itemValue(theItem)));
            printString(buf, mapToWindowX(60), min(ROWS-1, i + 1), &itemMessageColor, &black, dbuf);
            totalValue += max(0, itemValue(theItem));
            i++;
        }
	}
	i++;
	printString("TOTAL:", mapToWindowX(2), min(ROWS-1, i + 1), &lightBlue, &black, dbuf);
	sprintf(buf, "%li", totalValue);
	printString(buf, mapToWindowX(60), min(ROWS-1, i + 1), &lightBlue, &black, dbuf);
    
    i += 4;
    for (j = 0; i < ROWS && j < FEAT_COUNT; j++) {
        if (rogue.featRecord[j]) {
            sprintf(buf, "%s: %s", featTable[j].name, featTable[j].description);
            printString(buf, mapToWindowX(2), i, &advancementMessageColor, &black, dbuf);
            i++;
        }
    }
	
	funkyFade(dbuf, &white, 0, 120, COLS/2, ROWS/2, true);
	
    strcpy(victoryVerb, superVictory ? "Mastered" : "Escaped");
	if (gemCount == 0) {
		sprintf(theEntry.description, "%s the Dungeons of Doom!", victoryVerb);
	} else if (gemCount == 1) {
		sprintf(theEntry.description, "%s the Dungeons of Doom with a lumenstone!", victoryVerb);
	} else {
		sprintf(theEntry.description, "%s the Dungeons of Doom with %i lumenstones!", victoryVerb, gemCount);
	}
	
	theEntry.score = totalValue;
	
	if (rogue.easyMode) {
		theEntry.score /= 10;
	}
	
	if (!DEBUGGING && !rogue.playbackMode) {
		qualified = saveHighScore(theEntry);
	} else {
		qualified = false;
	}
	
	isPlayback = rogue.playbackMode;
	rogue.playbackMode = false;
	displayMoreSign();
	rogue.playbackMode = isPlayback;
	
	saveRecording();
	
	printHighScores(qualified);
	
	rogue.gameHasEnded = true;
}

void enableEasyMode() {
	if (rogue.easyMode) {
		message("Alas, all hope of salvation is lost. You shed scalding tears at your plight.", false);
		return;
	}
	message("A dark presence surrounds you, whispering promises of stolen power.", true);
	if (confirm("Succumb to demonic temptation (i.e. enable Easy Mode)?", false)) {
		recordKeystroke(EASY_MODE_KEY, false, true);
		message("An ancient and terrible evil burrows into your willing flesh!", true);
		player.info.displayChar = '&';
		rogue.easyMode = true;
		refreshDungeonCell(player.xLoc, player.yLoc);
		refreshSideBar(-1, -1, false);
		message("Wracked by spasms, your body contorts into an ALL-POWERFUL AMPERSAND!!!", false);
		message("You have a feeling that you will take 20% as much damage from now on.", false);
		message("But great power comes at a great price -- specifically, a 90% income tax rate.", false);
	} else {
		message("The evil dissipates, hissing, from the air around you.", false);
	}
}

// takes a flag of the form Fl(n) and returns n
short unflag(unsigned long flag) {
	short i;
	for (i=0; i<32; i++) {
		if (flag >> i == 1) {
			return i;
		}
	}
	return -1;
}
