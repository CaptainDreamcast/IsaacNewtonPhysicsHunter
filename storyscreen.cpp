#include "storyscreen.h"

#include <assert.h>

#include <prism/blitz.h>
#include <prism/stlutil.h>

#include "titlescreen.h"
#include <prism/soundeffect.h>
#include "gamescreen.h"
#include "titlescreen.h"

using namespace std;

static struct {
	MugenDefScript mScript;
	MugenDefScriptGroup* mCurrentGroup;
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;
	MugenSounds mSounds;

	MugenSounds mVoiceSounds;
	int mNextVoice = 0;

	MugenAnimation* mOldAnimation;
	MugenAnimation* mAnimation;
	MugenAnimationHandlerElement* mAnimationID;
	MugenAnimationHandlerElement* mOldAnimationID;

	Position mOldAnimationBasePosition;
	Position mAnimationBasePosition;

	Vector2D currentNamePos;
	Vector2D currentTextPos;

	int mSpeakerID;
	int mTextID;

	int mIsStoryOver;

	char mDefinitionPath[1024];
	int mTrack;
} gStoryScreenData;

static int isImageGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("image", firstW);
}

static void increaseGroup() {
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
}

static void loadImageGroup() {
	if (gStoryScreenData.mOldAnimationID != nullptr) {
		removeMugenAnimation(gStoryScreenData.mOldAnimationID);
		destroyMugenAnimation(gStoryScreenData.mOldAnimation);
	}

	if (gStoryScreenData.mAnimationID != nullptr) {
		setMugenAnimationBasePosition(gStoryScreenData.mAnimationID, &gStoryScreenData.mOldAnimationBasePosition);
	}

	gStoryScreenData.mOldAnimationID = gStoryScreenData.mAnimationID;
	gStoryScreenData.mOldAnimation = gStoryScreenData.mAnimation;

	int group = getMugenDefNumberVariableAsGroup(gStoryScreenData.mCurrentGroup, "group");
	int item = getMugenDefNumberVariableAsGroup(gStoryScreenData.mCurrentGroup, "item");
	gStoryScreenData.currentNamePos = getMugenDefVector2DOrDefaultAsGroup(gStoryScreenData.mCurrentGroup, "name.pos", Vector2D(30 / 2, 348 / 2));
	gStoryScreenData.currentTextPos = getMugenDefVector2DOrDefaultAsGroup(gStoryScreenData.mCurrentGroup, "text.pos", Vector2D(22, 175));
	gStoryScreenData.mAnimation = createOneFrameMugenAnimationForSprite(group, item);
	gStoryScreenData.mAnimationID = addMugenAnimation(gStoryScreenData.mAnimation, &gStoryScreenData.mSprites, Vector3D(0, 0, 0));
	setMugenAnimationBasePosition(gStoryScreenData.mAnimationID, &gStoryScreenData.mAnimationBasePosition);
	if (gStoryScreenData.mSpeakerID != -1) {
		setMugenTextPosition(gStoryScreenData.mSpeakerID, Vector3D(gStoryScreenData.currentNamePos.x, gStoryScreenData.currentNamePos.y, 3));
	}
	if (gStoryScreenData.mTextID != -1) {
		setMugenTextPosition(gStoryScreenData.mTextID, Vector3D(gStoryScreenData.currentTextPos.x, gStoryScreenData.currentTextPos.y, 3));
	}

	increaseGroup();
}


static int isTextGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("text", firstW);
}

static void loadTextGroup() {
	if (gStoryScreenData.mTextID != -1) {
		removeMugenText(gStoryScreenData.mTextID);
		removeMugenText(gStoryScreenData.mSpeakerID);
	}

	char* speaker = getAllocatedMugenDefStringVariableAsGroup(gStoryScreenData.mCurrentGroup, "speaker");
	char* text = getAllocatedMugenDefStringVariableAsGroup(gStoryScreenData.mCurrentGroup, "text");

	gStoryScreenData.mSpeakerID = addMugenText(speaker, Vector3D(gStoryScreenData.currentNamePos.x, gStoryScreenData.currentNamePos.y, 3), 1);

	gStoryScreenData.mTextID = addMugenText(text, Vector3D(gStoryScreenData.currentTextPos.x, gStoryScreenData.currentTextPos.y, 3), 1);
	setMugenTextBuildup(gStoryScreenData.mTextID, 1);
	setMugenTextTextBoxWidth(gStoryScreenData.mTextID, 530 / 2);
	setMugenTextColor(gStoryScreenData.mTextID, COLOR_BLACK);

	stopAllSoundEffects();
	tryPlayMugenSound(&gStoryScreenData.mVoiceSounds, 1, gStoryScreenData.mNextVoice);
	gStoryScreenData.mNextVoice++;

	freeMemory(speaker);
	freeMemory(text);

	increaseGroup();
}

static int isTitleGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("title", firstW);
}

static void goToTitle(void* tCaller) {
	(void)tCaller;
	setCurrentStoryDefinitionFile("game/INTRO.def", 1);
	setNewScreen(getTitleScreen());
}

static void loadTitleGroup() {
	gStoryScreenData.mIsStoryOver = 1;

	addFadeOut(30, goToTitle, NULL);
}

static int isGameGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("game", firstW);
}

static void goToGame(void* tCaller) {
	(void)tCaller;
	setNewScreen(getGameScreen());
}

static int isStoryGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("story", firstW);
}

static void goToStory(void* tCaller) {
	(void)tCaller;
	setCurrentStoryDefinitionFile("game/INTRO.def", 0);
	setNewScreen(getStoryScreen());
}

static void loadGameGroup() {
	gStoryScreenData.mIsStoryOver = 1;

	addFadeOut(30, goToGame, NULL);
}

static void loadStoryGroup() {
	gStoryScreenData.mIsStoryOver = 1;

	addFadeOut(30, goToStory, NULL);
}

static void loadNextStoryGroup() {
	int isRunning = 1;
	while (isRunning) {
		if (isImageGroup()) {
			loadImageGroup();
		}
		else if (isTextGroup()) {
			loadTextGroup();
			break;
		}
		else if (isTitleGroup()) {
			loadTitleGroup();
			break;
		}
		else if(isGameGroup())
		{
			loadGameGroup();
			break;
		}
		else if (isStoryGroup()) {
			loadStoryGroup();
			break;
		}
		else {
			logError("Unidentified group type.");
			//logErrorString(gStoryScreenData.mCurrentGroup->mName);
			abortSystem();
		}
	}
}

static void findStartOfStoryBoard() {
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mScript.mFirstGroup;

	while (gStoryScreenData.mCurrentGroup && "storystart" != gStoryScreenData.mCurrentGroup->mName) {
		gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
	}

	assert(gStoryScreenData.mCurrentGroup);
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
	assert(gStoryScreenData.mCurrentGroup);

	gStoryScreenData.mAnimationID = nullptr;
	gStoryScreenData.mOldAnimationID = nullptr;
	gStoryScreenData.mTextID = -1;
	gStoryScreenData.mSpeakerID = -1;

	gStoryScreenData.mOldAnimationBasePosition = Vector3D(0, 0, 1);
	gStoryScreenData.mAnimationBasePosition = Vector3D(0, 0, 2);

	loadNextStoryGroup();
}



static void loadStoryScreen() {
	gStoryScreenData.mNextVoice = 0;
	gStoryScreenData.mIsStoryOver = 0;

	if(isOnDreamcast())
	{
		gStoryScreenData.mSounds = loadMugenSoundFile("game/dreamcast/GAME.snd");
	}
	else
	{
		gStoryScreenData.mSounds = loadMugenSoundFile("game/GAME.snd");
	}

	std::string voicePath;
	if(isOnDreamcast())
	{
		voicePath = !strcmp(gStoryScreenData.mDefinitionPath, "game/INTRO.def") ? "game/dreamcast/INTRO.snd" : "game/dreamcast/OUTRO.snd";
	}
	else
	{
		voicePath = !strcmp(gStoryScreenData.mDefinitionPath, "game/INTRO.def") ? "game/INTRO.snd" : "game/OUTRO.snd";
	}
	gStoryScreenData.mVoiceSounds = loadMugenSoundFile(voicePath.c_str());


	loadMugenDefScript(&gStoryScreenData.mScript, gStoryScreenData.mDefinitionPath);

	{
		char path[1024];
		char folder[1024];
		strcpy(folder, gStoryScreenData.mDefinitionPath);
		char* dot = strrchr(folder, '.');
		*dot = '\0';
		sprintf(path, "%s.sff", folder);
		gStoryScreenData.mSprites = loadMugenSpriteFileWithoutPalette(path);
		sprintf(path, "%s.air", folder);
		gStoryScreenData.mAnimations = loadMugenAnimationFile(path);
	}

	findStartOfStoryBoard();

	if (isOnDreamcast())
	{
		setSoundEffectVolume(1.0);
	}
	else
	{
		setVolume(0.1);
		setSoundEffectVolume(1.0);
	}

	streamMusicFile("game/STORY.ogg");
	//playTrack(gStoryScreenData.mTrack);
}

static void playStoryExplosionEffect()
{
	auto newAnimation = addMugenAnimation(getMugenAnimation(&gStoryScreenData.mAnimations, 8000), &gStoryScreenData.mSprites, Vector3D(170, 120, 40));
	setMugenAnimationNoLoop(newAnimation);
	setMugenAnimationBaseDrawScale(newAnimation, 10);
}

static void updateText() {
	if (gStoryScreenData.mIsStoryOver) return;
	if (gStoryScreenData.mTextID == -1) return;

	if (hasPressedAFlankSingle(0) || hasPressedAFlankSingle(1) || hasPressedKeyboardKeyFlank(KEYBOARD_SPACE_PRISM) || hasPressedStartFlank() || hasPressedMouseLeftFlank()) {
		//tryPlayMugenSound(&gStoryScreenData.mSounds, 1, 2);
		if (isMugenTextBuiltUp(gStoryScreenData.mTextID)) {
			playStoryExplosionEffect();
			loadNextStoryGroup();
		}
		else {
			setMugenTextBuiltUp(gStoryScreenData.mTextID);
		}
	}
}

static void updateStoryScreen() {

	updateText();
}

static void unloadStoryScreen()
{
	gStoryScreenData.mTextID = -1;
	gStoryScreenData.mSpeakerID = -1;
	unloadMugenDefScript(&gStoryScreenData.mScript);
}

Screen gStoryScreen;

Screen* getStoryScreen() {
	gStoryScreen = makeScreen(loadStoryScreen, updateStoryScreen, nullptr, unloadStoryScreen);
	return &gStoryScreen;
}

void setCurrentStoryDefinitionFile(char* tPath, int tTrack) {
	strcpy(gStoryScreenData.mDefinitionPath, tPath);
	gStoryScreenData.mTrack = tTrack;
}
